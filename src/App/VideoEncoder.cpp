//
// Created by migue on 08/09/2026.
//

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "VideoEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <d3d11sdklayers.h>
#include "Logger.h"

static std::string avErr(const int code)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, buf, sizeof(buf));
    return buf;
}

/** @brief Intenta abrir un encoder H.264 concreto. Devuelve nullptr si no existe o falla al abrir. */
static AVCodecContext* openEncoder(const char* name, const int w, const int h)
{
    const AVCodec* codec = avcodec_find_encoder_by_name(name);
    if (!codec) {
        stdout_logger.log("(ENCODER) ", name, ": no incluido en esta build de FFmpeg.");
        return nullptr;
    }

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return nullptr;

    ctx->width        = w;
    ctx->height       = h;
    ctx->time_base    = AVRational{1, 60};
    ctx->framerate    = AVRational{60, 1};
    ctx->pix_fmt      = AV_PIX_FMT_NV12;
    ctx->bit_rate     = 8000000;
    ctx->gop_size     = 60;   // keyframe cada ~1 s a 60 fps
    ctx->max_b_frames = 0;    // sin B-frames: cero reordenamiento, minima latencia
    ctx->flags       |= AV_CODEC_FLAG_LOW_DELAY;
    // NO poner AV_CODEC_FLAG_GLOBAL_HEADER: queremos SPS/PPS en el propio stream (en cada IDR).

    // BT.709, rango limitado (tiene que coincidir con lo que hace sws en submitFrame)
    ctx->colorspace      = AVCOL_SPC_BT709;
    ctx->color_primaries = AVCOL_PRI_BT709;
    ctx->color_trc       = AVCOL_TRC_BT709;
    ctx->color_range     = AVCOL_RANGE_MPEG;

    if (std::strcmp(name, "h264_nvenc") == 0) {
        av_opt_set(ctx->priv_data, "preset", "p1", 0);
        av_opt_set(ctx->priv_data, "tune", "ull", 0);          // ultra low latency
        av_opt_set(ctx->priv_data, "rc", "cbr", 0);
        av_opt_set(ctx->priv_data, "profile", "baseline", 0);
        av_opt_set_int(ctx->priv_data, "zerolatency", 1, 0);
        av_opt_set_int(ctx->priv_data, "delay", 0, 0);
    } else if (std::strcmp(name, "h264_qsv") == 0) {
        av_opt_set(ctx->priv_data, "preset", "veryfast", 0);
        av_opt_set_int(ctx->priv_data, "async_depth", 1, 0);
    } else if (std::strcmp(name, "h264_amf") == 0) {
        av_opt_set(ctx->priv_data, "usage", "ultralowlatency", 0);
        av_opt_set(ctx->priv_data, "quality", "speed", 0);
    } else if (std::strcmp(name, "libx264") == 0) {
        av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(ctx->priv_data, "profile", "baseline", 0);
    }

    const int ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        stdout_logger.log("(ENCODER) ", name, ": avcodec_open2 fallo: ", avErr(ret));
        avcodec_free_context(&ctx);
        return nullptr;
    }
    return ctx;
}

bool VideoEncoder::init(ID3D11Device* device, ID3D11DeviceContext* context, const int w, const int h)
{
    d3dDevice  = device;
    d3dContext = context;
    width      = w;
    height     = h;

    if (!d3dDevice || !d3dContext || width <= 0 || height <= 0) {
        stderr_logger.log("(ENCODER) init(): parametros invalidos.");
        return false;
    }

    const char* candidates[] = { "h264_nvenc", "h264_qsv", "h264_amf", "libx264" };
    for (const char* name : candidates) {
        codecCtx = openEncoder(name, width, height);
        if (codecCtx) {
            stdout_logger.log("(ENCODER) Usando: ", codecCtx->codec->name);
            break;
        }
    }
    if (!codecCtx) {
        stderr_logger.log("(ENCODER) No se pudo abrir ningun encoder H.264.");
        return false;
    }

    frame  = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        stderr_logger.log("(ENCODER) No se pudo reservar AVFrame/AVPacket.");
        return false;
    }

    frame->format = AV_PIX_FMT_NV12;
    frame->width  = width;
    frame->height = height;
    const int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        stderr_logger.log("(ENCODER) av_frame_get_buffer fallo: ", avErr(ret));
        return false;
    }
    return true;
}

VideoEncoder::~VideoEncoder()
{
    if (sws)            sws_freeContext(sws);
    if (frame)          av_frame_free(&frame);
    if (packet)         av_packet_free(&packet);
    if (codecCtx)       avcodec_free_context(&codecCtx);
    if (stagingTexture) stagingTexture->Release();
}

/** @brief Asegura que existan la textura staging (lectura CPU) y el contexto sws para este formato. */
bool VideoEncoder::ensureStaging(const D3D11_TEXTURE2D_DESC& srcDesc)
{
    if (static_cast<int>(srcDesc.Width) != width || static_cast<int>(srcDesc.Height) != height) {
        stderr_logger.log("(ENCODER) La textura (", srcDesc.Width, "x", srcDesc.Height, ") no coincide con el encoder (", width, "x", height, ").");
        return false;
    }

    AVPixelFormat srcFmt;
    switch (srcDesc.Format) {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            srcFmt = AV_PIX_FMT_BGRA;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            srcFmt = AV_PIX_FMT_RGBA;
            break;
        default:
            stderr_logger.log("(ENCODER) Formato DXGI no soportado: ", static_cast<int>(srcDesc.Format), " (HDR / 10 bits no soportado en este camino).");
            return false;
    }

    if (stagingTexture && stagingFormat == srcDesc.Format && swsSrcFormat == static_cast<int>(srcFmt)) {
        return true;
    }

    if (stagingTexture) { stagingTexture->Release(); stagingTexture = nullptr; }
    if (sws)            { sws_freeContext(sws);      sws = nullptr; }

    D3D11_TEXTURE2D_DESC desc = srcDesc;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.BindFlags      = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags      = 0;
    desc.MipLevels      = 1;
    desc.ArraySize      = 1;

    if (FAILED(d3dDevice->CreateTexture2D(&desc, nullptr, &stagingTexture))) {
        stderr_logger.log("(ENCODER) No se pudo crear la textura staging.");
        stagingTexture = nullptr;
        return false;
    }
    stagingFormat = srcDesc.Format;

    sws = sws_getContext(width, height, srcFmt,
                         width, height, AV_PIX_FMT_NV12,
                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws) {
        stderr_logger.log("(ENCODER) sws_getContext fallo.");
        return false;
    }
    swsSrcFormat = static_cast<int>(srcFmt);

    // RGB entra en rango completo; NV12 sale en BT.709 rango limitado (coherente con ctx->color_range)
    const int* bt709 = sws_getCoefficients(SWS_CS_ITU709);
    sws_setColorspaceDetails(sws, bt709, 1, bt709, 0, 0, 1 << 16, 1 << 16);
    return true;
}

void VideoEncoder::drainPackets()
{
    if (!codecCtx) return;

    while (true) {
        const int ret = avcodec_receive_packet(codecCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            stderr_logger.log("(ENCODER) avcodec_receive_packet fallo: ", avErr(ret));
            break;
        }
        const char* data = reinterpret_cast<const char*>(packet->data);
        readyFrames.emplace(data, data + packet->size);
        av_packet_unref(packet);
    }
}

void VideoEncoder::update()
{
    drainPackets();
}

bool VideoEncoder::submitFrame(ID3D11Texture2D* bgraFrame, LONGLONG /*timestamp100ns*/)
{
    if (!codecCtx || !bgraFrame) return false;

    // Vaciar primero lo que ya este listo, para que send_frame no devuelva EAGAIN por acumulacion
    drainPackets();

    D3D11_TEXTURE2D_DESC desc;
    bgraFrame->GetDesc(&desc);
    if (!ensureStaging(desc)) return false;

    // GPU -> staging. El Map espera a que la GPU termine la copia (unos ms como mucho).
    d3dContext->CopyResource(stagingTexture, bgraFrame);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped))) {
        stderr_logger.log("(ENCODER) Map de la textura staging fallo.");
        return false;
    }

    int ret = av_frame_make_writable(frame);
    if (ret < 0) {
        d3dContext->Unmap(stagingTexture, 0);
        stderr_logger.log("(ENCODER) av_frame_make_writable fallo: ", avErr(ret));
        return false;
    }

    const uint8_t* srcData[4]   = { static_cast<const uint8_t*>(mapped.pData), nullptr, nullptr, nullptr };
    const int      srcStride[4] = { static_cast<int>(mapped.RowPitch), 0, 0, 0 };
    sws_scale(sws, srcData, srcStride, 0, height, frame->data, frame->linesize);

    d3dContext->Unmap(stagingTexture, 0);

    frame->pts = nextPts++;
    ret = avcodec_send_frame(codecCtx, frame);
    if (ret == AVERROR(EAGAIN)) {
        // El encoder no acepta mas input ahora: se descarta este frame (el siguiente tick captura uno mas nuevo)
        return false;
    }
    if (ret < 0) {
        stderr_logger.log("(ENCODER) avcodec_send_frame fallo: ", avErr(ret));
        return false;
    }

    drainPackets();
    return true;
}

bool VideoEncoder::popEncodedFrame(std::vector<char>& outFrame)
{
    if (readyFrames.empty()) return false;
    outFrame = std::move(readyFrames.front());
    readyFrames.pop();
    return true;
}

void VideoEncoder::dumpD3D11DebugMessages(ID3D11Device* device)
{
    ID3D11InfoQueue* infoQueue = nullptr;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) return;

    const UINT64 numMessages = infoQueue->GetNumStoredMessages();
    for (UINT64 i = 0; i < numMessages; ++i) {
        SIZE_T len = 0;
        infoQueue->GetMessage(i, nullptr, &len);
        std::vector<char> buffer(len);
        auto* msg = reinterpret_cast<D3D11_MESSAGE*>(buffer.data());
        infoQueue->GetMessage(i, msg, &len);
        stderr_logger.log("(D3D11 DEBUG) ", msg->pDescription);
    }
    infoQueue->ClearStoredMessages();
    infoQueue->Release();
}