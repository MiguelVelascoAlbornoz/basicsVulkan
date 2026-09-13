//
// Created by migue on 08/09/2026.
//

#include "VideoEncoder.h"
#include <iostream>      // <-- STL primero, siempre
#include <vector>

#include <wrl/client.h>

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
    frameDuration100ns = 10000000LL / fps;

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

    bool canContinue = false;

    for (UINT32 i = 0; i < count; ++i)
    {
        IMFActivate* candidate = activateArray[i];

        WCHAR name[256] = {};
        UINT32 len = 0;
        if (SUCCEEDED(candidate->GetString(MFT_FRIENDLY_NAME_Attribute, name, 256, &len))) {
            char narrowName[256] = {};
            WideCharToMultiByte(CP_UTF8, 0, name, -1, narrowName, sizeof(narrowName), nullptr, nullptr);
            std::cout << "MFT encontrado: " << narrowName << std::endl;
        }

        IMFTransform* candidateEncoder = nullptr;
        HRESULT hrActivate = candidate->ActivateObject(IID_PPV_ARGS(&candidateEncoder));

        if (SUCCEEDED(hrActivate) && candidateEncoder) {
            std::cout << "Encoder activado correctamente\n";
            encoderMFT = candidateEncoder;
            canContinue = true;
            break;
        }
        std::cerr << "ActivateObject fallo: 0x" << std::hex << hrActivate << std::dec << std::endl;
    }

    for (UINT32 i = 0; i < count; ++i) activateArray[i]->Release();
    CoTaskMemFree(activateArray);

    if (!canContinue) {
        std::cerr << "No se pudo activar ningun encoder\n";
        return false;
    }

    IMFAttributes* attrs = nullptr;
    hr = encoderMFT->GetAttributes(&attrs);
    if (FAILED(hr)) {
        std::cerr << "GetAttributes fallo: 0x" << std::hex << hr << std::dec << '\n';
        encoderMFT->Release(); encoderMFT = nullptr;
        return false;
    }

    UINT32 isD3D11Aware = 0;
    hr = attrs->GetUINT32(MF_SA_D3D11_AWARE, &isD3D11Aware);
    if (FAILED(hr)) {
        std::cerr << "MF_SA_D3D11_AWARE no disponible: 0x" << std::hex << hr << std::dec << '\n';
        attrs->Release();
        encoderMFT->Release(); encoderMFT = nullptr;
        return false;
    }
    std::cout << "Encoder D3D11 aware: " << isD3D11Aware << '\n';

    UINT32 async = 0;
    hr = attrs->GetUINT32(MF_TRANSFORM_ASYNC, &async);
    if (FAILED(hr)) {
        std::cerr << "MF_TRANSFORM_ASYNC no disponible: 0x" << std::hex << hr << std::dec << '\n';
        attrs->Release();
        encoderMFT->Release(); encoderMFT = nullptr;
        return false;
    }
    std::cout << "MF_TRANSFORM_ASYNC = " << async << '\n';

    if (async == 1) {
        hr = attrs->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
        if (FAILED(hr)) {
            std::cerr << "MF_TRANSFORM_ASYNC_UNLOCK fallo: 0x" << std::hex << hr << std::dec << '\n';
            attrs->Release();
            return false;
        }
    }
    attrs->Release();

    UINT resetToken = 0;
    hr = MFCreateDXGIDeviceManager(&resetToken, &dxgiDeviceManager);
    if (FAILED(hr) || !dxgiDeviceManager) {
        std::cerr << "MFCreateDXGIDeviceManager fallo: 0x" << std::hex << hr << std::endl;
        return false;
    }
    hr = dxgiDeviceManager->ResetDevice(device, resetToken);
    if (FAILED(hr)) {
        std::cerr << "ResetDevice fallo: 0x" << std::hex << hr << std::endl;
        return false;
    }

    if (isD3D11Aware) {
        hr = encoderMFT->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiDeviceManager));
        if (FAILED(hr)) {
            std::cerr << "SET_D3D_MANAGER fallo: 0x" << std::hex << hr << std::endl;
            return false;
        }
    } else {
        std::cerr << "(WARN) El encoder elegido no es D3D11-aware." << std::endl;
        return false;
    }

    if (!configureMediaTypes()) return false;

    // ---- Nuevo: engancharse al generador de eventos async del MFT ----
    hr = encoderMFT->QueryInterface(IID_PPV_ARGS(&eventGenerator));
    if (FAILED(hr) || !eventGenerator) {
        std::cerr << "No se pudo obtener IMFMediaEventGenerator del encoder: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    return initColorConverter();
}
bool VideoEncoder::configureMediaTypes() const
{
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

    ICodecAPI* codecApi = nullptr;
    encoderMFT->QueryInterface(IID_PPV_ARGS(&codecApi));
    if (codecApi) {
        VARIANT var;
        var.vt = VT_UI4; var.ulVal = 60; // GOP de 60 frames (~1s a 60fps): keyframe periodico
        codecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);

        var.vt = VT_BOOL; var.boolVal = VARIANT_TRUE;
        codecApi->SetValue(&CODECAPI_AVEncCommonLowLatency, &var);
        codecApi->Release();
    }

    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    return true;
}

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
    colorSpace.Usage         = 0;
    colorSpace.RGB_Range     = 0;
    colorSpace.YCbCr_Matrix  = 1;
    colorSpace.Nominal_Range = D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_0_255;
    videoContext->VideoProcessorSetStreamColorSpace(videoProcessor, 0, &colorSpace);
    videoContext->VideoProcessorSetOutputColorSpace(videoProcessor, &colorSpace);

    D3D11_TEXTURE2D_DESC nv12Desc = {};
    nv12Desc.Width  = width;
    nv12Desc.Height = height;
    nv12Desc.MipLevels = 1;
    nv12Desc.ArraySize = 1;
    nv12Desc.Format = DXGI_FORMAT_NV12;
    nv12Desc.SampleDesc.Count = 1;
    nv12Desc.Usage = D3D11_USAGE_DEFAULT;
    nv12Desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    if (FAILED(d3dDevice->CreateTexture2D(&nv12Desc, nullptr, &nv12Texture))) {
        std::cerr << "No se pudo crear la textura NV12." << std::endl;
        return false;
    }

    D3D11_TEXTURE2D_DESC inputDesc = {};
    inputDesc.Width  = width;
    inputDesc.Height = height;
    inputDesc.MipLevels = 1;
    inputDesc.ArraySize = 1;
    inputDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    inputDesc.SampleDesc.Count = 1;
    inputDesc.Usage = D3D11_USAGE_DEFAULT;
    inputDesc.BindFlags = 0;

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
        std::cerr << "VideoProcessorBlt fallo: 0x" << std::hex << hr << std::dec << std::endl;
        dumpD3D11DebugMessages(d3dDevice);
        return false;
    }
    return true;
}


VideoEncoder::~VideoEncoder()
{
    if (encoderMFT) {
        encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    }
    if (eventGenerator)     eventGenerator->Release();
    if (encoderMFT)         encoderMFT->Release();
    if (dxgiDeviceManager)  dxgiDeviceManager->Release();
    if (videoProcessor)     videoProcessor->Release();
    if (videoProcessorEnum) videoProcessorEnum->Release();
    if (videoContext)       videoContext->Release();
    if (videoDevice)        videoDevice->Release();
    if (nv12Texture)        nv12Texture->Release();
    if (encoderInputTexture) encoderInputTexture->Release();
}


void VideoEncoder::handleNeedInput()
{
    inputSlotsAvailable++;
}

void VideoEncoder::handleHaveOutput()
{
    MFT_OUTPUT_STREAM_INFO streamInfo = {};
    encoderMFT->GetOutputStreamInfo(0, &streamInfo);

    MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {};
    IMFSample* selfMadeSample = nullptr;

    bool mftProvidesSample = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
    if (!mftProvidesSample) {
        if (streamInfo.cbSize == 0) {
            std::cerr << "(ENCODER) cbSize=0 y el MFT no provee samples propios; se descarta este output." << std::endl;
            return;
        }
        IMFMediaBuffer* outBuffer = nullptr;
        MFCreateMemoryBuffer(streamInfo.cbSize, &outBuffer);
        MFCreateSample(&selfMadeSample);
        selfMadeSample->AddBuffer(outBuffer);
        outBuffer->Release();
    }
    outputDataBuffer.pSample = selfMadeSample; // nullptr si el MFT pone el suyo

    DWORD status = 0;
    HRESULT hr = encoderMFT->ProcessOutput(0, 1, &outputDataBuffer, &status);

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
        IMFMediaType* newType = nullptr;
        if (SUCCEEDED(encoderMFT->GetOutputAvailableType(0, 0, &newType))) {
            encoderMFT->SetOutputType(0, newType, 0);
            newType->Release();
        }
        if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();
        return;
    }
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
        // El evento decia que habia output, pero resulto que no -- puede pasar.
        if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();
        return;
    }
    if (FAILED(hr)) {
        std::cerr << "(ENCODER) ProcessOutput fallo: 0x" << std::hex << hr << std::dec << std::endl;
        if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();
        return;
    }
    //std::cout << "(ENCODER) ProcessOutput funciono: 0x" << std::hex << hr << std::dec << std::endl;
    IMFSample* sample = outputDataBuffer.pSample; // el nuestro, o el del MFT si PROVIDES_SAMPLES
    IMFMediaBuffer* dataBuffer = nullptr;
    sample->GetBufferByIndex(0, &dataBuffer);

    BYTE* rawData = nullptr;
    DWORD rawLen = 0;
    dataBuffer->Lock(&rawData, nullptr, &rawLen);

    readyFrames.emplace(reinterpret_cast<char*>(rawData), reinterpret_cast<char*>(rawData) + rawLen);

    dataBuffer->Unlock();
    dataBuffer->Release();
    sample->Release();
}

void VideoEncoder::update()
{
    if (!eventGenerator) return;

    while (true) {
        IMFMediaEvent* event = nullptr;
        HRESULT hr = eventGenerator->GetEvent(MF_EVENT_FLAG_NO_WAIT, &event);

        if (hr == MF_E_NO_EVENTS_AVAILABLE) break;
        if (FAILED(hr)) {
            std::cerr << "(ENCODER) GetEvent fallo: 0x" << std::hex << hr << std::dec << std::endl;
            break;
        }

        MediaEventType type = MEUnknown;
        event->GetType(&type);

        HRESULT eventStatus = S_OK;
        event->GetStatus(&eventStatus);
        event->Release();

        if (FAILED(eventStatus)) {
            std::cerr << "(ENCODER) Evento con status de error: 0x" << std::hex << eventStatus << std::dec << std::endl;
            continue;
        }

        switch (type) {
            case METransformNeedInput:
                handleNeedInput();
                break;
            case METransformHaveOutput:
                handleHaveOutput();
                break;
            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------
// API publica por tick
// ---------------------------------------------------------------------

bool VideoEncoder::submitFrame(ID3D11Texture2D* bgraFrame, LONGLONG timestamp100ns)
{
    if (inputSlotsAvailable <= 0) {
        // El MFT todavia no pidio mas input este tick -> se descarta la captura.
        return false;
    }

    if (!convertToNV12(bgraFrame)) {
        return false;
    }

    IMFMediaBuffer* buffer = nullptr;
    if (FAILED(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), nv12Texture, 0, FALSE, &buffer))) {
        std::cerr << "(ENCODER) No se pudo crear el DXGI surface buffer." << std::endl;
        return false;
    }

    IMFSample* inputSample = nullptr;
    MFCreateSample(&inputSample);
    inputSample->AddBuffer(buffer);
    inputSample->SetSampleTime(timestamp100ns);
    inputSample->SetSampleDuration(frameDuration100ns);
    buffer->Release();

    HRESULT hr = encoderMFT->ProcessInput(0, inputSample, 0);
    inputSample->Release();

    if (FAILED(hr)) {
        std::cerr << "(ENCODER) ProcessInput fallo: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    //std::cout << "Process input funciono: " << std::hex << hr << std::dec << std::endl;
    inputSlotsAvailable--;
    return true;
}

bool VideoEncoder::popEncodedFrame(std::vector<char>& outFrame)
{
    if (readyFrames.empty()) return false;
    outFrame = std::move(readyFrames.front());
    readyFrames.pop();
    return true;
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
