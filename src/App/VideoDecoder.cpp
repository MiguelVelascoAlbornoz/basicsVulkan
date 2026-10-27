// VideoDecoder.cpp
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "VideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <dxgi1_2.h>
#include "Logger.h"

static std::string avErr(const int code)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, buf, sizeof(buf));
    return buf;
}

bool VideoDecoder::init(ID3D11Device* device, ID3D11DeviceContext* context, const int w, const int h)
{
    if (!device || !context || w <= 0 || h <= 0) {
        stderr_logger.log("(DECODER) init(): parametros invalidos.");
        return false;
    }

    d3dDevice  = device;
    d3dContext = context;
    width      = w;
    height     = h;

    // Para usar NVDEC en vez de CPU basta con probar antes "h264_cuvid" con avcodec_find_decoder_by_name().
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        stderr_logger.log("(DECODER) Esta build de FFmpeg no incluye decoder H.264.");
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        stderr_logger.log("(DECODER) avcodec_alloc_context3 fallo.");
        return false;
    }
    codecCtx->flags      |= AV_CODEC_FLAG_LOW_DELAY;   // no retener frames esperando reordenamiento
    codecCtx->thread_type  = FF_THREAD_SLICE;          // el frame-threading anade latencia de varios frames
    codecCtx->thread_count = 0;                        // auto

    const int ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        stderr_logger.log("(DECODER) avcodec_open2 fallo: ", avErr(ret));
        avcodec_free_context(&codecCtx);
        return false;
    }
    decoderName = codec->name;

    frame  = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        stderr_logger.log("(DECODER) No se pudo reservar AVFrame/AVPacket.");
        return false;
    }

    bgraBuffer.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    if (!createSharedOutputTexture()) return false;

    stdout_logger.log("(DECODER) Listo: ", decoderName, " ", width, "x", height);
    return true;
}

VideoDecoder::~VideoDecoder()
{
    if (sws)                 sws_freeContext(sws);
    if (frame)               av_frame_free(&frame);
    if (packet)              av_packet_free(&packet);
    if (codecCtx)            avcodec_free_context(&codecCtx);
    if (keyedMutex)          keyedMutex->Release();
    if (sharedOutputTexture) sharedOutputTexture->Release();
    // OJO: Image::importFromD3D11Handle() ya cierra este handle al importarlo.
    // Si se importo, poner sharedHandle = nullptr antes de destruir el decoder para no cerrarlo dos veces.
    if (sharedHandle)        CloseHandle(sharedHandle);
}

/** @brief Convierte un frame YUV decodificado a BGRA en bgraBuffer (reescalando si el tamano no coincide). */
bool VideoDecoder::convertToBgra(const AVFrame* src)
{
    sws = sws_getCachedContext(sws,
                               src->width, src->height, static_cast<AVPixelFormat>(src->format),
                               width, height, AV_PIX_FMT_BGRA,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws) {
        stderr_logger.log("(DECODER) sws_getCachedContext fallo.");
        return false;
    }

    // Matriz y rango segun lo que declare el stream (el encoder lo marca como BT.709 / limitado)
    const bool isBt601 = (src->colorspace == AVCOL_SPC_BT470BG || src->colorspace == AVCOL_SPC_SMPTE170M);
    const int* srcCoeffs = sws_getCoefficients(isBt601 ? SWS_CS_ITU601 : SWS_CS_ITU709);
    const int  srcRange  = (src->color_range == AVCOL_RANGE_JPEG) ? 1 : 0;
    sws_setColorspaceDetails(sws, srcCoeffs, srcRange, sws_getCoefficients(SWS_CS_DEFAULT), 1, 0, 1 << 16, 1 << 16);

    uint8_t* dstData[4]   = { bgraBuffer.data(), nullptr, nullptr, nullptr };
    const int dstStride[4] = { width * 4, 0, 0, 0 };
    sws_scale(sws, src->data, src->linesize, 0, src->height, dstData, dstStride);
    return true;
}

ID3D11Texture2D* VideoDecoder::decodeFrame(const char* data, const int size)
{
    if (!codecCtx || !data || size <= 0) return nullptr;

    // 1. Entrada: av_new_packet reserva con el padding que exige FFmpeg
    int ret = av_new_packet(packet, size);
    if (ret < 0) {
        stderr_logger.log("(DECODER) av_new_packet fallo: ", avErr(ret));
        return nullptr;
    }
    std::memcpy(packet->data, data, static_cast<size_t>(size));
    ret = avcodec_send_packet(codecCtx, packet);
    av_packet_unref(packet);
    if (ret < 0) {
        stderr_logger.log("(DECODER) avcodec_send_packet fallo: ", avErr(ret));
        return nullptr;
    }

    // 2. Salida: drenar TODO lo que haya (normalmente 0 o 1 frame). Nos quedamos con el ultimo.
    bool gotFrame = false;
    while (true) {
        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            stderr_logger.log("(DECODER) avcodec_receive_frame fallo: ", avErr(ret));
            break;
        }
        if (convertToBgra(frame)) gotFrame = true;
    }
    if (!gotFrame) return nullptr; // normal: el decoder todavia no tiene frame completo

    // 3. Escribir en la textura compartida bajo el protocolo del keyed mutex:
    //    D3D11 (escribe): acquire key 0 / release key 1.  Vulkan (lee): acquire key 1 / release key 0.
    const HRESULT hr = keyedMutex->AcquireSync(0, 1000);
    if (hr != S_OK) {
        stderr_logger.log("(DECODER) keyedMutex->AcquireSync fallo: 0x", std::hex, hr, std::dec);
        return nullptr;
    }

    d3dContext->UpdateSubresource(sharedOutputTexture, 0, nullptr, bgraBuffer.data(),
                                  static_cast<UINT>(width * 4), 0);

    keyedMutex->ReleaseSync(0);

    return sharedOutputTexture;
}

bool VideoDecoder::createSharedOutputTexture()
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = static_cast<UINT>(width);
    desc.Height           = static_cast<UINT>(height);
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = 0; // solo se escribe con UpdateSubresource
    desc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    if (FAILED(d3dDevice->CreateTexture2D(&desc, nullptr, &sharedOutputTexture))) {
        stderr_logger.log("No se pudo crear la textura compartida de salida.");
        return false;
    }

    if (FAILED(sharedOutputTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&keyedMutex)))) {
        stderr_logger.log("No se pudo obtener el keyed mutex de la textura compartida.");
        return false;
    }

    IDXGIResource1* resource1 = nullptr;
    if (FAILED(sharedOutputTexture->QueryInterface(__uuidof(IDXGIResource1), reinterpret_cast<void**>(&resource1)))) {
        stderr_logger.log("No se pudo obtener IDXGIResource1.");
        return false;
    }
    const HRESULT hr = resource1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &sharedHandle);
    resource1->Release();

    if (FAILED(hr)) {
        stderr_logger.log("No se pudo crear el shared handle.");
        return false;
    }
    return true;
}