// VideoDecoder.h
#ifndef BASICSVULKAN_VIDEODECODER_H
#define BASICSVULKAN_VIDEODECODER_H

// STL primero (GCC 16 -Wtemplate-body + -Werror)
#include <cstdint>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>

// Forward declarations de FFmpeg para no arrastrar sus headers (C) a quien incluya esto.
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

/**
 * @brief Decoder H.264 basado en FFmpeg (libavcodec, decoder software "h264").
 * @details Recibe un access unit H.264 (Annex B) por llamada, lo decodifica, lo convierte a BGRA
 * con swscale y lo escribe en sharedOutputTexture (compartida con Vulkan) respetando el
 * protocolo del keyed mutex: D3D11 acquire key 0 / release key 1; Vulkan acquire key 1 / release key 0.
 */
class VideoDecoder
{
public:
    VideoDecoder() = default;
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);

    ID3D11Texture2D* sharedOutputTexture = nullptr; // la que se comparte con Vulkan
    IDXGIKeyedMutex* keyedMutex = nullptr;
    HANDLE sharedHandle = nullptr;

    bool createSharedOutputTexture();

    /** @brief Mete bytes H.264 (un access unit) y, si sale un frame decodificado, lo escribe en
     *  sharedOutputTexture y la devuelve. Devuelve nullptr si todavia no hay frame o si algo fallo. */
    ID3D11Texture2D* decodeFrame(const char* data, int size);

    std::string decoderName;

private:
    bool convertToBgra(const AVFrame* src);

    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;     // frame decodificado, reutilizable
    AVPacket* packet = nullptr;   // paquete de entrada, reutilizable
    SwsContext* sws = nullptr;    // YUV -> BGRA (se recrea solo si cambia el formato/tamano de entrada)

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

    int width = 0, height = 0;
    std::vector<uint8_t> bgraBuffer; // width * height * 4, destino del swscale
};

#endif