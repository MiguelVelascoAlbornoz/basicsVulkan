//
// Created by migue on 08/09/2026.
//

#ifndef BASICSVULKAN_VIDEOENCODER_H
#define BASICSVULKAN_VIDEOENCODER_H


#include <vector>

#define WIN32_LEAN_AND_MEAN

#include <d3d11.h>
#include "mfobjects.h"
#include "mftransform.h"
class VideoEncoder
{

public:
    std::vector<char> encodeFrame(ID3D11Texture2D* bgraFrame, LONGLONG timestamp100ns);
    IMFDXGIDeviceManager* dxgiDeviceManager;
    VideoEncoder() : dxgiDeviceManager(nullptr), width(0), height(0), encoderMFT(nullptr)
    {
    } ;
    private:
    int width, height;
    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
    bool configureMediaTypes() const;
    IMFTransform * encoderMFT;
    ID3D11VideoDevice* videoDevice = nullptr;
    ID3D11VideoContext* videoContext = nullptr;
    ID3D11VideoProcessor* videoProcessor = nullptr;
    ID3D11VideoProcessorEnumerator* videoProcessorEnum = nullptr;
    ID3D11Texture2D* nv12Texture = nullptr; // textura intermedia BGRA->NV12
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    bool initColorConverter();
    bool convertToNV12(ID3D11Texture2D* bgraSource) const;

    LONGLONG frameDuration100ns = 0;
};


#endif //BASICSVULKAN_VIDEOENCODER_H
