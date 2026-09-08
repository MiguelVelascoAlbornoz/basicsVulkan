// VideoDecoder.h
#ifndef BASICSVULKAN_VIDEODECODER_H
#define BASICSVULKAN_VIDEODECODER_H

#include <d3d11.h>
#include "mfobjects.h"
#include "mftransform.h"

class VideoDecoder
{
public:
    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
    /** @brief Mete bytes H.264 crudos y devuelve, si ya hay un frame listo,
     *  la textura BGRA decodificada (nullptr si el decoder todavía está acumulando). */
    ID3D11Texture2D* sharedOutputTexture = nullptr; // la que se comparte con Vulkan
    IDXGIKeyedMutex* keyedMutex = nullptr;
    HANDLE sharedHandle = nullptr;

    bool createSharedOutputTexture();
    ID3D11Texture2D* decodeFrame(const char* data, int size) const;

private:
    IMFTransform* decoderMFT = nullptr;
    IMFDXGIDeviceManager* dxgiDeviceManager = nullptr;
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

    ID3D11VideoDevice* videoDevice = nullptr;
    ID3D11VideoContext* videoContext = nullptr;
    ID3D11VideoProcessor* videoProcessor = nullptr;
    ID3D11VideoProcessorEnumerator* videoProcessorEnum = nullptr;
    ID3D11Texture2D* bgraOutputTexture = nullptr; // resultado final, listo para Vulkan

    int width = 0, height = 0;
    bool configureMediaTypes() const;
    bool initColorConverter(); // NV12 -> BGRA, inverso del encoder
    ID3D11Texture2D* convertNV12ToBGRA(ID3D11Texture2D* nv12Source) const;
};

#endif