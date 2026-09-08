// VideoDecoder.cpp
#include "VideoDecoder.h"

#include <dxgi1_2.h>
#include <mfapi.h>
#include <mferror.h>
#include <iostream>

bool VideoDecoder::init(ID3D11Device* device, ID3D11DeviceContext* context, int w, int h)
{
    d3dDevice = device; d3dContext = context; width = w; height = h;
    MFStartup(MF_VERSION);

    MFT_REGISTER_TYPE_INFO inputInfo = { MFMediaType_Video, MFVideoFormat_H264 };
    IMFActivate** activateArray = nullptr;
    UINT32 count = 0;

    MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER,
              MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
              &inputInfo, nullptr, &activateArray, &count);

    if (count == 0) {
        std::cerr << "No hay decoder H.264 de hardware disponible." << std::endl;
        return false;
    }
    activateArray[0]->ActivateObject(IID_PPV_ARGS(&decoderMFT));
    for (UINT32 i = 0; i < count; i++) activateArray[i]->Release();
    CoTaskMemFree(activateArray);

    UINT resetToken = 0;
    MFCreateDXGIDeviceManager(&resetToken, &dxgiDeviceManager);
    dxgiDeviceManager->ResetDevice(d3dDevice, resetToken);
    decoderMFT->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiDeviceManager));

    if (!configureMediaTypes()) return false;
    if (!initColorConverter()) return false;
    return createSharedOutputTexture();
}

bool VideoDecoder::configureMediaTypes() const
{
    // INPUT primero en el decoder (al revés que el encoder)
    IMFMediaType* inputType = nullptr;
    MFCreateMediaType(&inputType);
    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(inputType, MF_MT_FRAME_SIZE, width, height);
    decoderMFT->SetInputType(0, inputType, 0);
    inputType->Release();

    IMFMediaType* outputType = nullptr;
    MFCreateMediaType(&outputType);
    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);

    if (FAILED(decoderMFT->SetOutputType(0, outputType, 0))) {
        std::cerr << "El decoder no acepta NV12 de salida." << std::endl;
        outputType->Release();
        return false;
    }
    outputType->Release();

    decoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    decoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    return true;
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
    return bgra; // el caller decide qué hacer con ella (copiar a tu textura compartida)
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