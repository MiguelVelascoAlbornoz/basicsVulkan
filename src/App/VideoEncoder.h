//
// Created by migue on 08/09/2026.
//

#ifndef BASICSVULKAN_VIDEOENCODER_H
#define BASICSVULKAN_VIDEOENCODER_H

#include <d3d11.h>

#include "mfobjects.h"
#include "mftransform.h"
class VideoEncoder
{

public:
    IMFDXGIDeviceManager* dxgiDeviceManager;

    private:
    int width, height;
    bool init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
    bool configureMediaTypes() const;
    IMFTransform * encoderMFT;
};


#endif //BASICSVULKAN_VIDEOENCODER_H
