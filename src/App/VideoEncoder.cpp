//
// Created by migue on 08/09/2026.
//

#include "VideoEncoder.h"

#include <mfapi.h>
#include <mftransform.h>
#include <mferror.h>
#include <codecapi.h>
#include <d3d11.h>
#include <iostream>
#include "icodecapi.h"

bool VideoEncoder::init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
    MFStartup(MF_VERSION);

    // 1. Enumerar y activar el primer encoder H.264 de hardware disponible
    MFT_REGISTER_TYPE_INFO outputInfo = { MFMediaType_Video, MFVideoFormat_H264 };
    IMFActivate** activateArray = nullptr;
    UINT32 count = 0;

    MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
              MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
              nullptr, &outputInfo, &activateArray, &count);

    if (count == 0) {
        std::cerr << "No hay encoder H.264 de hardware disponible." << std::endl;
        return false;
    }

    activateArray[0]->ActivateObject(IID_PPV_ARGS(&encoderMFT));
    for (UINT32 i = 0; i < count; i++) activateArray[i]->Release();
    CoTaskMemFree(activateArray);

    // 2. Darle acceso al mismo D3D11 device que usa Desktop Duplication
    //    (esto es lo que evita copias CPU<->GPU)
    UINT resetToken = 0;
    MFCreateDXGIDeviceManager(&resetToken, &dxgiDeviceManager);
    dxgiDeviceManager->ResetDevice(device, resetToken);

    encoderMFT->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiDeviceManager));

    this->width = width;
    this->height = height;
    return configureMediaTypes();
}
bool VideoEncoder::configureMediaTypes() const
{
    // ---- OUTPUT type primero (el MFT lo exige en este orden) ----
    IMFMediaType* outputType = nullptr;
    MFCreateMediaType(&outputType);
    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outputType->SetUINT32(MF_MT_AVG_BITRATE, 8000000); // 8 Mbps, ajustable
    MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(outputType, MF_MT_FRAME_RATE, 60, 1);
    outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    outputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base); // baseline, menor latencia

    encoderMFT->SetOutputType(0, outputType, 0);
    outputType->Release();

    // ---- INPUT type: NV12 (lo que espera el encoder) ----
    IMFMediaType* inputType = nullptr;
    MFCreateMediaType(&inputType);
    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(inputType, MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(inputType, MF_MT_FRAME_RATE, 60, 1);
    inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    if (FAILED(encoderMFT->SetInputType(0, inputType, 0))) {
        std::cerr << "El encoder no acepta NV12 directo, revisar formatos soportados." << std::endl;
        inputType->Release();
        return false;
    }
    inputType->Release();

    // ---- Config de baja latencia: sin B-frames, GOP corto ----
    ICodecAPI* codecApi = nullptr;
    encoderMFT->QueryInterface(IID_PPV_ARGS(&codecApi));
    if (codecApi) {
        VARIANT var;
        var.vt = VT_UI4; var.ulVal = 0;
        codecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &var); // o un GOP corto, ej. 60

        var.vt = VT_BOOL; var.boolVal = VARIANT_FALSE;
        codecApi->SetValue(&CODECAPI_AVEncCommonLowLatency, &var);
        var.boolVal = VARIANT_TRUE;
        codecApi->SetValue(&CODECAPI_AVEncCommonLowLatency, &var);

        codecApi->Release();
    }

    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    return true;
}
