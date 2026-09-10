//
// Created by migue on 08/09/2026.
//

#include "VideoEncoder.h"
#include <iostream>      // <-- STL primero, siempre
#include <vector>



#define WIN32_LEAN_AND_MEAN
#include <mfapi.h>
#include <mftransform.h>
#include <mferror.h>
#include <codecapi.h>
#include "icodecapi.h"
bool VideoEncoder::init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
    this->d3dDevice = device;
    this->d3dContext = context;
    this->width = width;
    this->height = height;

    constexpr int fps = 60;
    frameDuration100ns = 10000000LL / fps; // 10,000,000 = 1 segundo en unidades de 100ns

    MFStartup(MF_VERSION);

    MFT_REGISTER_TYPE_INFO outputInfo = { MFMediaType_Video, MFVideoFormat_H264 };
    IMFActivate** activateArray = nullptr;
    UINT32 count = 0;

    HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
      MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
      nullptr, &outputInfo, &activateArray, &count);
    if (FAILED(hr) || count == 0) {
        std::cerr << "No hay encoder H.264 de hardware disponible." << std::endl;
        return false;
    }
    //IMFTransform* encoderMFT = nullptr;
    bool canContinue = false;

    for (UINT32 i = 0; i < count; ++i)
    {
        IMFActivate* candidate = activateArray[i];

        WCHAR name[256] = {};
        UINT32 len = 0;

        HRESULT hrName = candidate->GetString(
            MFT_FRIENDLY_NAME_Attribute,
            name,
            256,
            &len
        );

        if (SUCCEEDED(hrName))
        {
            char narrowName[256] = {};
            WideCharToMultiByte(CP_UTF8, 0, name, -1, narrowName, sizeof(narrowName), nullptr, nullptr);
            std::cout << "MFT encontrado: " << narrowName << std::endl;
        }




        // Intentar activar ESTE encoder
        IMFTransform* candidateEncoder = nullptr;

        HRESULT hr = candidate->ActivateObject(
            IID_PPV_ARGS(&candidateEncoder)
        );

        if (SUCCEEDED(hr) && candidateEncoder)
        {
            std::cout << "Encoder activado correctamente\n";

            encoderMFT = candidateEncoder;
            canContinue = true;

            break;
        }

        std::cerr << "ActivateObject fallo: 0x"
                  << std::hex << hr << std::dec << std::endl;
    }

    IMFAttributes* attrs = nullptr;

    hr = encoderMFT->GetAttributes(&attrs);

    if (FAILED(hr))
    {
        std::cerr << "GetAttributes fallo: 0x"
                  << std::hex << hr << std::dec << '\n';
        return false;
    }

    UINT32 isD3D11Aware = 0;

    hr = attrs->GetUINT32(
        MF_SA_D3D11_AWARE,
        &isD3D11Aware
    );

    if (FAILED(hr))
    {
        std::cerr << "MF_SA_D3D11_AWARE no disponible: 0x"
                  << std::hex << hr << std::dec << '\n';

        attrs->Release();
        return false;
    }

    std::cout << "Encoder D3D11 aware: "<< isD3D11Aware << '\n';

    UINT32 async = 0;

    hr = attrs->GetUINT32(
       MF_TRANSFORM_ASYNC,
       &async
   );

    std::cout << "MF_TRANSFORM_ASYNC = "
              << async << '\n';

    if (async == 1)
    {
        hr = attrs->SetUINT32(
    MF_TRANSFORM_ASYNC_UNLOCK,
    TRUE);

        if (FAILED(hr))
        {
            std::cerr << "MF_TRANSFORM_ASYNC_UNLOCK fallo: 0x"
                      << std::hex << hr << std::dec << '\n';
            return false;
        }
    }

    attrs->Release();
    // Ahora sí liberar TODOS los IMFActivate
    for (UINT32 i = 0; i < count; ++i)
    {
        activateArray[i]->Release();
    }

    CoTaskMemFree(activateArray);

    if (!canContinue)
    {
        std::cerr << "No se pudo activar ningun encoder\n";
        return false;
    }

    UINT resetToken = 0;
    hr = MFCreateDXGIDeviceManager(&resetToken, &dxgiDeviceManager);
    if (FAILED(hr) || !dxgiDeviceManager) {
        std::cerr << "MFCreateDXGIDeviceManager falló: 0x" << std::hex << hr << std::endl;
        return false;
    }
    IDXGIDevice* dxgiDevice = nullptr;

     hr = d3dDevice->QueryInterface(
        IID_PPV_ARGS(&dxgiDevice)
    );

    std::cout << "ID3D11Device -> IDXGIDevice: 0x"
              << std::hex << hr << std::dec << '\n';

    if (SUCCEEDED(hr))
        dxgiDevice->Release();
    hr = dxgiDeviceManager->ResetDevice(device, resetToken);
    if (FAILED(hr)) {
        std::cerr << "ResetDevice falló: 0x" << std::hex << hr << std::endl;
        return false;
    }
    std::cout << "encoderMFT = " << encoderMFT << '\n';
    std::cout << "dxgiDeviceManager = " << dxgiDeviceManager << '\n';
    std::cout << "isD3D11Aware = " << isD3D11Aware << '\n';
    if (isD3D11Aware) {
        hr = encoderMFT->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiDeviceManager));
        if (FAILED(hr)) {
            std::cerr << "SET_D3D_MANAGER falló: 0x" << std::hex << hr << std::endl;
            return false;
        }
    } else {
        std::cerr << "(WARN) El encoder elegido no es D3D11-aware; el pipeline de zero-copy no va a funcionar con este MFT." << std::endl;
        return false; // o intenta buscar otro candidato en activateArray si count > 1
    }

    if (!configureMediaTypes()) return false;
    if (!configureMediaTypes()) return false;

    // Faltaba: sin esto convertToNV12() truena por punteros nulos.
    return initColorConverter();
}
bool VideoEncoder::configureMediaTypes() const
{
    // OUTPUT primero (el MFT lo exige en este orden)
    IMFMediaType* outputType = nullptr;
    MFCreateMediaType(&outputType);
    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outputType->SetUINT32(MF_MT_AVG_BITRATE, 8000000);
    MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(outputType, MF_MT_FRAME_RATE, 60, 1);
    outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    outputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);

    HRESULT hrOut = encoderMFT->SetOutputType(0, outputType, 0);
    outputType->Release();
    if (FAILED(hrOut)) {
        std::cerr << "SetOutputType fallo: 0x" << std::hex << hrOut << std::dec << std::endl;
        return false;
    }

    IMFMediaType* inputType = nullptr;
    MFCreateMediaType(&inputType);
    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(inputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(inputType, MF_MT_FRAME_RATE, 60, 1);
    inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    HRESULT hrIn = encoderMFT->SetInputType(0, inputType, 0);
    inputType->Release();
    if (FAILED(hrIn)) {
        std::cerr << "El encoder no acepta NV12 directo: 0x" << std::hex << hrIn << std::dec << std::endl;
        return false;
    }

    IMFMediaType* currentOutputType = nullptr;
    if (SUCCEEDED(encoderMFT->GetOutputCurrentType(0, &currentOutputType))) {
        UINT32 w = 0, h = 0;
        MFGetAttributeSize(currentOutputType, MF_MT_FRAME_SIZE, &w, &h);
        GUID subtype;
        currentOutputType->GetGUID(MF_MT_SUBTYPE, &subtype);
        std::cout << "Output type activo: " << w << "x" << h
                  << " subtype=" << (subtype == MFVideoFormat_H264 ? "H264" : "OTRO") << std::endl;
        currentOutputType->Release();
    } else {
        std::cerr << "GetOutputCurrentType fallo tras SetInputType." << std::endl;
    }

    MFT_OUTPUT_STREAM_INFO checkInfo = {};
    encoderMFT->GetOutputStreamInfo(0, &checkInfo);
    std::cout << "Post-SetOutputType/SetInputType: cbSize=" << checkInfo.cbSize
              << " dwFlags=0x" << std::hex << checkInfo.dwFlags << std::dec << std::endl;

    ICodecAPI* codecApi = nullptr;
    encoderMFT->QueryInterface(IID_PPV_ARGS(&codecApi));
    if (codecApi) {
        VARIANT var;
        var.vt = VT_UI4; var.ulVal = 0;
        codecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);

        var.vt = VT_BOOL; var.boolVal = VARIANT_TRUE;
        codecApi->SetValue(&CODECAPI_AVEncCommonLowLatency, &var);
        codecApi->Release();
    }

    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    return true;
}
// VideoEncoder.cpp

bool VideoEncoder::initColorConverter()
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
    contentDesc.InputWidth  = width;
    contentDesc.InputHeight = height;
    contentDesc.OutputWidth  = width;
    contentDesc.OutputHeight = height;
    contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

    if (FAILED(videoDevice->CreateVideoProcessorEnumerator(&contentDesc, &videoProcessorEnum))) {
        std::cerr << "No se pudo crear el video processor enumerator." << std::endl;
        return false;
    }
    UINT flags = 0;
    HRESULT hr = videoProcessorEnum->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM, &flags);
    std::cout << "CheckVideoProcessorFormat hr=0x" << std::hex << hr
             << " flags=0x" << flags << std::dec << std::endl;
    if (FAILED(videoDevice->CreateVideoProcessor(videoProcessorEnum, 0, &videoProcessor))) {
        std::cerr << "No se pudo crear el video processor." << std::endl;
        return false;
    }
    RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    videoContext->VideoProcessorSetStreamFrameFormat(videoProcessor, 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
    videoContext->VideoProcessorSetStreamSourceRect(videoProcessor, 0, TRUE, &rect);
    videoContext->VideoProcessorSetStreamDestRect(videoProcessor, 0, TRUE, &rect);
    videoContext->VideoProcessorSetOutputTargetRect(videoProcessor, TRUE, &rect);
    D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace = {};
    colorSpace.Usage         = 0; // 0 = playback, 1 = video processing
    colorSpace.RGB_Range     = 0; // 0 = full range [0,255]
    colorSpace.YCbCr_Matrix  = 1; // 1 = BT.709 (contenido HD), 0 = BT.601
    colorSpace.Nominal_Range = D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_0_255;

    videoContext->VideoProcessorSetStreamColorSpace(videoProcessor, 0, &colorSpace);
    videoContext->VideoProcessorSetOutputColorSpace(videoProcessor, &colorSpace);
    // Textura destino NV12, la que realmente le entra al encoder
    D3D11_TEXTURE2D_DESC nv12Desc = {};
    nv12Desc.Width  = width;
    nv12Desc.Height = height;
    nv12Desc.MipLevels = 1;
    nv12Desc.ArraySize = 1;
    nv12Desc.Format = DXGI_FORMAT_NV12;
    nv12Desc.SampleDesc.Count = 1;
    nv12Desc.Usage = D3D11_USAGE_DEFAULT;
    nv12Desc.BindFlags = D3D11_BIND_RENDER_TARGET; // el video processor escribe como si fuera un RT

    if (FAILED(d3dDevice->CreateTexture2D(&nv12Desc, nullptr, &nv12Texture))) {
        std::cerr << "No se pudo crear la textura NV12." << std::endl;
        return false;
    }
    D3D11_TEXTURE2D_DESC inputDesc = {};
    inputDesc.Width  = width;
    inputDesc.Height = height;
    inputDesc.MipLevels = 1;
    inputDesc.ArraySize = 1;
    inputDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // mismo formato que el desktop capturado
    inputDesc.SampleDesc.Count = 1;
    inputDesc.Usage = D3D11_USAGE_DEFAULT;
    inputDesc.BindFlags = 0; // suficiente para servir de input view
    // SIN MiscFlags de shared/keyed mutex

    if (FAILED(d3dDevice->CreateTexture2D(&inputDesc, nullptr, &encoderInputTexture))) {
        std::cerr << "No se pudo crear la textura de entrada del encoder." << std::endl;
        return false;
    }
    return true;
}

bool VideoEncoder::convertToNV12(ID3D11Texture2D* bgraSource) const
{

    d3dContext->CopyResource(encoderInputTexture, bgraSource);

    ID3D11VideoProcessorInputView* inputView = nullptr;
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inDesc = {};
    inDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inDesc.Texture2D.MipSlice = 0;
    HRESULT hrInput = videoDevice->CreateVideoProcessorInputView(encoderInputTexture, videoProcessorEnum, &inDesc, &inputView);
    if (FAILED(hrInput)) {
        std::cerr << "No se pudo crear input view: 0x" << std::hex << hrInput << std::dec << std::endl;
        dumpD3D11DebugMessages(d3dDevice);
        return false;
    }

    ID3D11VideoProcessorOutputView* outputView = nullptr;
    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outDesc = {};
    outDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    if (FAILED(videoDevice->CreateVideoProcessorOutputView(nv12Texture, videoProcessorEnum, &outDesc, &outputView))) {
        std::cerr << "No se pudo crear output view del video processor." << std::endl;
        inputView->Release();
        return false;
    }

    D3D11_VIDEO_PROCESSOR_STREAM stream = {};
    stream.Enable = TRUE;
    stream.pInputSurface = inputView;

    HRESULT hr = videoContext->VideoProcessorBlt(videoProcessor, outputView, 0, 1, &stream);

    inputView->Release();
    outputView->Release();

    if (FAILED(hr)) {
        std::cerr << "VideoProcessorBlt falló: 0x" << std::hex << hr << std::dec << std::endl;
        dumpD3D11DebugMessages(d3dDevice);
        return false;
    }
    return true;
}
// VideoEncoder.cpp

std::vector<char> VideoEncoder::encodeFrame(ID3D11Texture2D* bgraFrame, LONGLONG timestamp100ns) const
{
    std::vector<char> result;

    if (!convertToNV12(bgraFrame)) {
        return result;
    }

    // 1. Envolver la textura NV12 (ya en GPU) en un IMFSample, sin copiar a RAM
    IMFMediaBuffer* buffer = nullptr;
    if (FAILED(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), nv12Texture, 0, FALSE, &buffer))) {
        std::cerr << "No se pudo crear el DXGI surface buffer." << std::endl;
        return result;
    }

    IMFSample* inputSample = nullptr;
    MFCreateSample(&inputSample);
    inputSample->AddBuffer(buffer);
    inputSample->SetSampleTime(timestamp100ns);
    inputSample->SetSampleDuration(frameDuration100ns);
    buffer->Release();

    // 2. Meter el frame al encoder
    HRESULT hr = encoderMFT->ProcessInput(0, inputSample, 0);


    if (hr == MF_E_NOTACCEPTING) {
        drainEncoderOutput(result);
        hr = encoderMFT->ProcessInput(0, inputSample, 0);
    }
    std::cout << "ProcessInput result: 0x" << std::hex << hr << std::dec << std::endl;

    inputSample->Release();

    if (FAILED(hr)) {
        std::cerr << "ProcessInput falló: 0x" << std::hex << hr << std::dec << std::endl;
        return result;
    }

    // 3. Sacar todo el output disponible (puede ser 0, 1 o más samples)
    drainEncoderOutput(result);
    return result;
}
void VideoEncoder::drainEncoderOutput(std::vector<char>& result) const
{
    while (true) {
        MFT_OUTPUT_STREAM_INFO streamInfo = {};
        encoderMFT->GetOutputStreamInfo(0, &streamInfo);

        MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {};
        IMFSample* outputSample = nullptr;

        if (!(streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)) {
            IMFMediaBuffer* outBuffer = nullptr;
            MFCreateMemoryBuffer(streamInfo.cbSize, &outBuffer);
            MFCreateSample(&outputSample);
            outputSample->AddBuffer(outBuffer);
            outBuffer->Release();
        }
        outputDataBuffer.pSample = outputSample;

        DWORD status = 0;
        HRESULT hr = encoderMFT->ProcessOutput(0, 1, &outputDataBuffer, &status);


        if (FAILED(hr) && hr != MF_E_TRANSFORM_NEED_MORE_INPUT) {
            std::cerr << "ProcessOutput falló: 0x" << std::hex << hr << std::dec
                       << " streamInfo.dwFlags=0x" << std::hex << streamInfo.dwFlags
                       << " cbSize=" << std::dec << streamInfo.cbSize << std::endl;
        }
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            if (outputSample) outputSample->Release();
            break;
        }
        if (FAILED(hr)) {
            if (outputSample) outputSample->Release();
            std::cerr << "ProcessOutput falló: 0x" << std::hex << hr << std::dec << std::endl;
            break;
        }

        IMFMediaBuffer* dataBuffer = nullptr;
        outputDataBuffer.pSample->GetBufferByIndex(0, &dataBuffer);

        BYTE* rawData = nullptr;
        DWORD rawLen = 0;
        dataBuffer->Lock(&rawData, nullptr, &rawLen);

        size_t offset = result.size();
        result.resize(offset + rawLen);
        memcpy(result.data() + offset, rawData, rawLen);

        dataBuffer->Unlock();
        dataBuffer->Release();
        outputDataBuffer.pSample->Release();
    }
}
#include <d3d11sdklayers.h>

void VideoEncoder::dumpD3D11DebugMessages(ID3D11Device* device)
{
    ID3D11InfoQueue* infoQueue = nullptr;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) return;

    UINT64 numMessages = infoQueue->GetNumStoredMessages();
    for (UINT64 i = 0; i < numMessages; ++i) {
        SIZE_T len = 0;
        infoQueue->GetMessage(i, nullptr, &len);
        std::vector<char> buffer(len);
        auto* msg = reinterpret_cast<D3D11_MESSAGE*>(buffer.data());
        infoQueue->GetMessage(i, msg, &len);
        std::cerr << "(D3D11 DEBUG) " << msg->pDescription << std::endl;
    }
    infoQueue->ClearStoredMessages();
    infoQueue->Release();
}
