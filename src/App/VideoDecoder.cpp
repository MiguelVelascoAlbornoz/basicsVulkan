// VideoDecoder.cpp
#include "VideoDecoder.h"

#include <dxgi1_2.h>
#include <mfapi.h>
#include <mferror.h>
#include <iostream>
auto logOk = [](const std::string& text) {
    std::cout << "[VideoDecoder] OK: " << text << '\n';
};

auto logInfo = [](const std::string& text) {
    std::cout << "[VideoDecoder] INFO: " << text << '\n';
};

auto logHr = [](const char* operation, HRESULT hr) {
    std::cerr << "[VideoDecoder] ERROR: " << operation
              << " fallo (HRESULT=0x"
              << std::hex << static_cast<unsigned long>(hr)
              << std::dec << ")\n";
};
bool VideoDecoder::init(ID3D11Device* device, ID3D11DeviceContext* context, int w, int h)
{
    if (!device || !context || w <= 0 || h <= 0)
        return false;

    d3dDevice = device;
    d3dContext = context;
    width = w;
    height = h;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "(DECODER) MFStartup fallo: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    MFT_REGISTER_TYPE_INFO inputInfo{ MFMediaType_Video, MFVideoFormat_H264 };
    IMFActivate** activates = nullptr;
    UINT32 count = 0;

    const UINT32 enumFlags =
        MFT_ENUM_FLAG_HARDWARE |
        MFT_ENUM_FLAG_SYNCMFT |
        MFT_ENUM_FLAG_ASYNCMFT |
        MFT_ENUM_FLAG_LOCALMFT |
        MFT_ENUM_FLAG_SORTANDFILTER;

    hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, enumFlags, &inputInfo, nullptr, &activates, &count);
    if (FAILED(hr) || count == 0) {
        std::cerr << "(DECODER) No se encontro ningun decoder H.264: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    // --- Preparar el device manager ANTES de tocar cualquier candidato ---
    bool setupOk = true;
    UINT resetToken = 0;

    hr = MFCreateDXGIDeviceManager(&resetToken, &dxgiDeviceManager);
    if (FAILED(hr)) {
        std::cerr << "(DECODER) MFCreateDXGIDeviceManager fallo: 0x" << std::hex << hr << std::dec << '\n';
        setupOk = false;
    }

    if (setupOk) {
        hr = dxgiDeviceManager->ResetDevice(d3dDevice, resetToken);
        if (FAILED(hr)) {
            std::cerr << "(DECODER) ResetDevice fallo: 0x" << std::hex << hr << std::dec << '\n';
            setupOk = false;
        }
    }

    bool asyncSelected = false;

    if (setupOk) {
        for (UINT32 i = 0; i < count && !decoderMFT; ++i) {
            IMFTransform* candidate = nullptr;
            if (FAILED(activates[i]->ActivateObject(IID_PPV_ARGS(&candidate))))
                continue;

            // --- DEBUG: nombre del candidato ---
            WCHAR name[256] = {};
            UINT32 nameLen = 0;
            char narrowName[256] = "(sin nombre)";
            if (SUCCEEDED(activates[i]->GetString(MFT_FRIENDLY_NAME_Attribute, name, 256, &nameLen))) {
                WideCharToMultiByte(CP_UTF8, 0, name, -1, narrowName, sizeof(narrowName), nullptr, nullptr);
            }
            std::cout << "(DECODER) MFT candidato: " << narrowName << std::endl;

            IMFAttributes* attrs = nullptr;
            UINT32 d3d11Aware = FALSE;
            UINT32 async = FALSE;
            HRESULT hrCandidate = S_OK;

            if (SUCCEEDED(candidate->GetAttributes(&attrs))) {
                attrs->GetUINT32(MF_SA_D3D11_AWARE, &d3d11Aware);
                attrs->GetUINT32(MF_TRANSFORM_ASYNC, &async);


                // --- DEBUG: aware y async de este candidato ---
                std::cout << "(DECODER)   D3D11 aware: " << (d3d11Aware ? "si" : "no")
                          << " | Asincrono: " << (async ? "si" : "no") << std::endl;


                // CRITICO: el unlock va ANTES de cualquier otra llamada al MFT
                // (incluido SET_D3D_MANAGER). Si esto se salta, todo lo demas
                // falla con 0xc00d6d77 y el candidato se descarta sin motivo real.
                if (async) {
                    hrCandidate = attrs->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
                }
            } else {
                std::cerr << "(DECODER)   GetAttributes fallo para este candidato." << std::endl;
                hrCandidate = E_FAIL;
            }

            if (SUCCEEDED(hrCandidate) && d3d11Aware) {
                hrCandidate = candidate->ProcessMessage(
                    MFT_MESSAGE_SET_D3D_MANAGER,
                    reinterpret_cast<ULONG_PTR>(dxgiDeviceManager));
                if (FAILED(hrCandidate)) {
                    std::cerr << "(DECODER)   SET_D3D_MANAGER fallo: 0x" << std::hex << hrCandidate << std::dec << std::endl;
                }
            }

            if (attrs) attrs->Release();

            if (SUCCEEDED(hrCandidate) && d3d11Aware) {
                decoderMFT = candidate;
                asyncSelected = (async != 0);

                WCHAR name[256] = {};
                UINT32 nameLen = 0;
                if (SUCCEEDED(activates[i]->GetString(MFT_FRIENDLY_NAME_Attribute, name, 256, &nameLen))) {
                    char narrowName[256] = {};
                    WideCharToMultiByte(CP_UTF8, 0, name, -1, narrowName, sizeof(narrowName), nullptr, nullptr);
                    std::cout << "(DECODER) MFT seleccionado: " << narrowName << std::endl;
                }
                break;
            }

            candidate->Release();
        }
    }

    for (UINT32 i = 0; i < count; ++i)
        activates[i]->Release();
    CoTaskMemFree(activates);

    if (!setupOk || !decoderMFT) {
        std::cerr << "(DECODER) No se encontro un decoder H.264 compatible con D3D11.\n";
        return false;
    }

    if (!configureMediaTypes())
        return false;

    // Solo un MFT asincrono necesita IMFMediaEventGenerator
    if (asyncSelected) {
        hr = decoderMFT->QueryInterface(IID_PPV_ARGS(&eventGenerator));
        if (FAILED(hr)) {
            std::cerr << "(DECODER) No se pudo obtener IMFMediaEventGenerator: 0x" << std::hex << hr << std::dec << '\n';
            return false;
        }
    }

    std::cout << "(DECODER) Media Foundation inicializado." << std::endl;
    std::cout << "(DECODER) Asincrono: " << (asyncSelected ? "si" : "no") << std::endl;

    if (!initColorConverter())        return false;
    if (!createSharedOutputTexture()) return false;

    std::cout << "\n========================================\n"
              << "[VideoDecoder] Decoder listo\n"
              << "  Resolucion: " << width << "x" << height << '\n'
              << "  Async:      " << (asyncSelected ? "si" : "no") << '\n'
              << "========================================\n";

    return true;
}
bool VideoDecoder::configureMediaTypes() const
{
    IMFMediaType* inputType = nullptr;
    MFCreateMediaType(&inputType);
    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(inputType, MF_MT_FRAME_SIZE, width, height);
    HRESULT hrIn = decoderMFT->SetInputType(0, inputType, 0);
    inputType->Release();
    if (FAILED(hrIn)) {
        std::cerr << "SetInputType fallo: 0x" << std::hex << hrIn << std::dec << std::endl;
        return false;
    }

    // DEBUG: confirmar que el input type quedó comprometido
    IMFMediaType* committedIn = nullptr;
    if (SUCCEEDED(decoderMFT->GetInputCurrentType(0, &committedIn))) {
        GUID sub; committedIn->GetGUID(MF_MT_SUBTYPE, &sub);
        std::cout << "(DECODER DEBUG) Input comprometido: "
                  << (sub == MFVideoFormat_H264 ? "H264 OK" : "INESPERADO") << std::endl;
        committedIn->Release();
    } else {
        std::cerr << "(DECODER DEBUG) GetInputCurrentType fallo tras SetInputType." << std::endl;
    }

    IMFMediaType* outputType = nullptr;
    MFCreateMediaType(&outputType);
    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);

    HRESULT hrOut = decoderMFT->SetOutputType(0, outputType, 0);
    outputType->Release();
    if (FAILED(hrOut)) {
        std::cerr << "El decoder no acepta NV12: 0x" << std::hex << hrOut << std::dec << std::endl;
        return false;
    }

    // DEBUG: confirmar output type + stream info
    IMFMediaType* committedOut = nullptr;
    if (SUCCEEDED(decoderMFT->GetOutputCurrentType(0, &committedOut))) {
        GUID sub; committedOut->GetGUID(MF_MT_SUBTYPE, &sub);
        std::cout << "(DECODER DEBUG) Output comprometido: "
                  << (sub == MFVideoFormat_NV12 ? "NV12 OK" : "INESPERADO") << std::endl;
        committedOut->Release();
    } else {
        std::cerr << "(DECODER DEBUG) GetOutputCurrentType fallo tras SetOutputType." << std::endl;
    }

    MFT_OUTPUT_STREAM_INFO outInfo{};
    if (SUCCEEDED(decoderMFT->GetOutputStreamInfo(0, &outInfo))) {
        std::cout << "(DECODER DEBUG) OutputStreamInfo cbSize=" << outInfo.cbSize
                  << " dwFlags=0x" << std::hex << outInfo.dwFlags << std::dec << std::endl;
    }

    decoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    decoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    return true;
}
VideoDecoder::~VideoDecoder()
{
    if (eventGenerator)      eventGenerator->Release();
    if (decoderMFT)          decoderMFT->Release();
    if (dxgiDeviceManager)   dxgiDeviceManager->Release();
    if (videoProcessor)      videoProcessor->Release();
    if (videoProcessorEnum)  videoProcessorEnum->Release();
    if (videoContext)        videoContext->Release();
    if (videoDevice)         videoDevice->Release();
    if (bgraOutputTexture)   bgraOutputTexture->Release();
    if (sharedOutputTexture) sharedOutputTexture->Release();
    if (keyedMutex)          keyedMutex->Release();
    if (sharedHandle)        CloseHandle(sharedHandle);
}
ID3D11Texture2D* VideoDecoder::decodeFrame(const char* data, int size) const
{
    // 1. Meter los bytes recibidos en un IMFSample (aquí sí hay copia CPU->GPU,
    //    porque llega por red como bytes planos, no hay forma de evitarlo)
    IMFMediaBuffer* inBuffer = nullptr;
    MFCreateMemoryBuffer(size, &inBuffer);

    BYTE* rawPtr = nullptr;
    inBuffer->Lock(&rawPtr, nullptr, nullptr);
    memcpy(rawPtr, data, size);
    inBuffer->Unlock();
    inBuffer->SetCurrentLength(size);

    IMFSample* inputSample = nullptr;
    MFCreateSample(&inputSample);
    inputSample->AddBuffer(inBuffer);
    inBuffer->Release();

    HRESULT hr = decoderMFT->ProcessInput(0, inputSample, 0);
    inputSample->Release();

    if (FAILED(hr)) {
        std::cerr << "Decoder ProcessInput falló: 0x" << std::hex << hr << std::endl;
        return nullptr;
    }

    // 2. Intentar sacar un frame decodificado
    MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {};
    DWORD status = 0;
    hr = decoderMFT->ProcessOutput(0, 1, &outputDataBuffer, &status);

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
        // El decoder detectó cambio de formato (típico en el primer keyframe real).
        // Hay que releer el media type de salida y reintentar.
        IMFMediaType* newType = nullptr;
        decoderMFT->GetOutputAvailableType(0, 0, &newType);
        decoderMFT->SetOutputType(0, newType, 0);
        newType->Release();
        return nullptr;
    }
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT || FAILED(hr)) {
        return nullptr; // normal, todavía no hay frame completo
    }

    // 3. Obtener la textura D3D11 (NV12) que trae el sample - esto sí es zero-copy
    IMFMediaBuffer* outBuffer = nullptr;
    outputDataBuffer.pSample->GetBufferByIndex(0, &outBuffer);

    IMFDXGIBuffer* dxgiBuffer = nullptr;
    outBuffer->QueryInterface(IID_PPV_ARGS(&dxgiBuffer));

    ID3D11Texture2D* nv12Texture = nullptr;
    dxgiBuffer->GetResource(IID_PPV_ARGS(&nv12Texture));

    dxgiBuffer->Release();
    outBuffer->Release();
    outputDataBuffer.pSample->Release();

    // 4. Convertir NV12 -> BGRA (mismo VideoProcessorBlt, en la dirección opuesta)
    ID3D11Texture2D* bgra = convertNV12ToBGRA(nv12Texture);
    nv12Texture->Release();
    if (!bgra) return nullptr;

    // --- NUEVO: copiar a la textura compartida bajo el protocolo del keyed mutex ---
    // Convención ya establecida en el proyecto:
    //   D3D11 (este lado, quien escribe) acquire key 0 / release key 1
    //   Vulkan (quien lee)               acquire key 1 / release key 0
     hr = keyedMutex->AcquireSync(0, 1000);
    if (hr != S_OK) {
        std::cerr << "(DECODER) keyedMutex->AcquireSync fallo: 0x" << std::hex << hr << std::dec << std::endl;
        return nullptr;
    }

    d3dContext->CopyResource(sharedOutputTexture, bgra);

    keyedMutex->ReleaseSync(1);

    return sharedOutputTexture; // el caller ya no necesita tocar bgraOutputTexture directamente
}
// VideoDecoder.cpp

bool VideoDecoder::initColorConverter()
{
    if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&videoDevice)))) {
        std::cerr << "No se pudo obtener ID3D11VideoDevice." << std::endl;
        return false;
    }
    if (FAILED(d3dContext->QueryInterface(IID_PPV_ARGS(&videoContext)))) {
        std::cerr << "No se pudo obtener ID3D11VideoContext." << std::endl;
        return false;
    }

    D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {};
    contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
    contentDesc.InputWidth   = width;
    contentDesc.InputHeight  = height;
    contentDesc.OutputWidth  = width;
    contentDesc.OutputHeight = height;
    contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

    if (FAILED(videoDevice->CreateVideoProcessorEnumerator(&contentDesc, &videoProcessorEnum))) {
        std::cerr << "No se pudo crear el video processor enumerator (decoder)." << std::endl;
        return false;
    }
    if (FAILED(videoDevice->CreateVideoProcessor(videoProcessorEnum, 0, &videoProcessor))) {
        std::cerr << "No se pudo crear el video processor (decoder)." << std::endl;
        return false;
    }

    // Textura destino BGRA: esta es la que luego importas a Vulkan con
    // importFromD3D11Handle(), igual que ya haces con desktopImage.
    D3D11_TEXTURE2D_DESC bgraDesc = {};
    bgraDesc.Width  = width;
    bgraDesc.Height = height;
    bgraDesc.MipLevels = 1;
    bgraDesc.ArraySize = 1;
    bgraDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bgraDesc.SampleDesc.Count = 1;
    bgraDesc.Usage = D3D11_USAGE_DEFAULT;
    bgraDesc.BindFlags = D3D11_BIND_RENDER_TARGET; // el video processor escribe como si fuera un RT

    if (FAILED(d3dDevice->CreateTexture2D(&bgraDesc, nullptr, &bgraOutputTexture))) {
        std::cerr << "No se pudo crear la textura BGRA de salida." << std::endl;
        return false;
    }
    return true;
}

ID3D11Texture2D* VideoDecoder::convertNV12ToBGRA(ID3D11Texture2D* nv12Source) const
{
    ID3D11VideoProcessorInputView* inputView = nullptr;
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inDesc = {};
    inDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inDesc.Texture2D.MipSlice = 0;
    if (FAILED(videoDevice->CreateVideoProcessorInputView(nv12Source, videoProcessorEnum, &inDesc, &inputView))) {
        std::cerr << "No se pudo crear input view del video processor (decoder)." << std::endl;
        return nullptr;
    }

    ID3D11VideoProcessorOutputView* outputView = nullptr;
    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outDesc = {};
    outDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    if (FAILED(videoDevice->CreateVideoProcessorOutputView(bgraOutputTexture, videoProcessorEnum, &outDesc, &outputView))) {
        std::cerr << "No se pudo crear output view del video processor (decoder)." << std::endl;
        inputView->Release();
        return nullptr;
    }

    D3D11_VIDEO_PROCESSOR_STREAM stream = {};
    stream.Enable = TRUE;
    stream.pInputSurface = inputView;

    HRESULT hr = videoContext->VideoProcessorBlt(videoProcessor, outputView, 0, 1, &stream);

    inputView->Release();
    outputView->Release();

    if (FAILED(hr)) {
        std::cerr << "VideoProcessorBlt falló (decoder)." << std::endl;
        return nullptr;
    }
    return bgraOutputTexture;
}
bool VideoDecoder::createSharedOutputTexture()
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width  = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = 0; // no hace falta bind, solo se copia hacia ella
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    if (FAILED(d3dDevice->CreateTexture2D(&desc, nullptr, &sharedOutputTexture))) {
        std::cerr << "No se pudo crear la textura compartida de salida." << std::endl;
        return false;
    }

    if (FAILED(sharedOutputTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex))) {
        std::cerr << "No se pudo obtener el keyed mutex de la textura compartida." << std::endl;
        return false;
    }

    IDXGIResource1* resource1 = nullptr;
    if (FAILED(sharedOutputTexture->QueryInterface(__uuidof(IDXGIResource1), (void**)&resource1))) {
        std::cerr << "No se pudo obtener IDXGIResource1." << std::endl;
        return false;
    }
    HRESULT hr = resource1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &sharedHandle);
    resource1->Release();

    if (FAILED(hr)) {
        std::cerr << "No se pudo crear el shared handle." << std::endl;
        return false;
    }
    return true;
}