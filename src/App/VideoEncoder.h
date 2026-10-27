//
// Created by migue on 08/09/2026.
//

#ifndef BASICSVULKAN_VIDEOENCODER_H
#define BASICSVULKAN_VIDEOENCODER_H

// STL primero (GCC 16 -Wtemplate-body + -Werror)
#include <cstdint>
#include <queue>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>

// Forward declarations de FFmpeg para no arrastrar sus headers (C) a quien incluya esto.
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

/**
 * @brief Encoder H.264 basado en FFmpeg (libavcodec).
 * @details Prueba, en orden: h264_nvenc, h264_qsv, h264_amf y libx264 (software).
 * El flujo por tick es el mismo que antes: update() -> submitFrame() -> while(popEncodedFrame()).
 * Camino de datos: textura D3D11 BGRA -> staging (CPU) -> swscale a NV12 -> encoder.
 * El stream sale en Annex B con SPS/PPS repetidos en cada IDR (un cliente puede unirse tarde).
 */
class VideoEncoder
{
public:
    VideoEncoder() = default;
    ~VideoEncoder();

    VideoEncoder(const VideoEncoder&) = delete;
    VideoEncoder& operator=(const VideoEncoder&) = delete;

    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);

    /** @brief Mete el frame capturado al encoder.
     *  @details Si el encoder no puede aceptar mas input en este momento (EAGAIN),
     *  el frame se descarta: para streaming en vivo es preferible a bloquear.
     *  El timestamp se ignora; el encoder usa un contador de frames como pts.
     *  @return true si el frame entro al encoder, false si se descarto o fallo. */
    bool submitFrame(ID3D11Texture2D* bgraFrame, LONGLONG timestamp100ns);

    /** @brief Recoge los paquetes que el encoder ya tenga listos y los encola.
     *  Se mantiene por compatibilidad con App.cpp (llamar una vez por tick, antes de submitFrame). */
    void update();

    /** @brief Saca un paquete H.264 completo (un access unit) de la cola interna, si hay alguno. */
    bool popEncodedFrame(std::vector<char>& outFrame);

    static void dumpD3D11DebugMessages(ID3D11Device* device);

private:
    bool ensureStaging(const D3D11_TEXTURE2D_DESC& srcDesc);
    void drainPackets();

    int width = 0, height = 0;

    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;     // frame NV12 reutilizable
    AVPacket* packet = nullptr;   // paquete reutilizable
    SwsContext* sws = nullptr;    // BGRA/RGBA -> NV12
    int swsSrcFormat = -1;        // AVPixelFormat con el que se creo sws
    int64_t nextPts = 0;

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    ID3D11Texture2D* stagingTexture = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;

    /** @brief Paquetes comprimidos listos para enviar, uno por access unit. */
    std::queue<std::vector<char>> readyFrames;
};

#endif //BASICSVULKAN_VIDEOENCODER_H