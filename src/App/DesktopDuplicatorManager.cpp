//
// Created by migue on 29/08/2026.
//

#include "DesktopDuplicatorManager.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <iostream>
#include <vector>
#include "Logger.h"

bool DesktopDuplicatorManager::createDesktopDuplicator(){
    if (!initializeID3D11())
    {
        return false;
    }
    if (!selectDuplicationOuput())
    {
        return false;
    }
    if (!createDestinyResource())
    {
        return false;
    }
    if (!createEncoderCaptureTexture())
    {
        return false;
    }
    if (!createWindowsHandler())
    {
        return false;
    }
    return true;
}

DesktopDuplicatorManager::~DesktopDuplicatorManager()
{
    // dstResource depende del device
    if (dstResource)       { dstResource->Release();       dstResource = nullptr; }
    if (frameTexture) {            frameTexture->Release(); frameTexture = nullptr; }
    if (encoderCaptureTexture) { encoderCaptureTexture->Release(); encoderCaptureTexture = nullptr; }
    // outputDuplication depende del device
    if (outputDuplication) { outputDuplication->Release(); outputDuplication = nullptr; }

    if (context) { context->Release(); context = nullptr; }
    if (device)  { device->Release();  device = nullptr; }

    // OJO con esto, ver punto 2
    if (handle) {  handle = nullptr; }
}

// ... createDestinyResource() queda exactamente igual ...

bool DesktopDuplicatorManager::createEncoderCaptureTexture()
{
    if (width == 0 || height == 0)
    {
        stderr_logger.log("createDestinyResource() should be called before createEncoderCaptureTexture().");
        return false;
    }

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width            = width;
    textureDesc.Height           = height;
    textureDesc.Format           = format;
    textureDesc.MipLevels        = 1;
    textureDesc.ArraySize        = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage            = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags        = 0;
    textureDesc.MiscFlags        = 0; // <-- SIN shared/keyed mutex, es de uso exclusivo local

    if (device->CreateTexture2D(&textureDesc, nullptr, &encoderCaptureTexture) != S_OK)
    {
        stderr_logger.log("No se pudo crear la textura de captura para el encoder.");
        return false;
    }
    return true;
}

// ... createWindowsHandler() queda igual ...

bool DesktopDuplicatorManager::writeDestinyResource() const
{
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* frameResource = nullptr;

    HRESULT hr = outputDuplication->AcquireNextFrame(500, &frameInfo, &frameResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return false; // normal, sin cambios en pantalla
    }
    if (hr != S_OK) {
        stderr_logger.log("Error obtaining desktop frame: 0x", std::hex, hr);
        return false;
    }

    // A partir de acá tenemos SIEMPRE un frameResource pendiente de liberar
    // (tanto el COM ref como el ReleaseFrame() del duplication object).

    //HRESULT mutexResult = keyedMutex->AcquireSync(0, 1000);
    //if (mutexResult != S_OK) {
        //stderr_logger.log("[RAMA FALLO] antes de ReleaseFrame");
    //    frameResource->Release();
     //   outputDuplication->ReleaseFrame();
        //stderr_logger.log("[RAMA FALLO] despues de ReleaseFrame");
    //    return false;
    //}

    bool gotFrame = false;
    ID3D11Texture2D* localFrameTexture = nullptr;
    if (frameResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&localFrameTexture) == S_OK) {
        context->CopyResource(dstResource, localFrameTexture);
        context->CopyResource(encoderCaptureTexture, localFrameTexture);
        localFrameTexture->Release();
        gotFrame = true;
    } else {
        stderr_logger.log("No se pudo obtener la desktop texture");
    }

    frameResource->Release();
    //stderr_logger.log("[RAMA EXITO] antes de ReleaseFrame");
    outputDuplication->ReleaseFrame();
    //stderr_logger.log("[RAMA EXITO] despues de ReleaseFrame");

    //keyedMutex->ReleaseSync(1); // esto sí corresponde: acá SÍ lo adquirimos arriba

    return gotFrame;
}

bool DesktopDuplicatorManager::createDestinyResource()
{
    if (outputDuplication == nullptr)
    {
        stderr_logger.log("selectOutputDuplication() should be called before.");
        return false;
    }

    //Now copy de frameTexturo into a new Texture
    //Crear la textra de destino
    DXGI_OUTDUPL_DESC outputDesc;
    outputDuplication->GetDesc(&outputDesc);
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width            = outputDesc.ModeDesc.Width;
    textureDesc.Height           = outputDesc.ModeDesc.Height;
    textureDesc.Format           = outputDesc.ModeDesc.Format;
    textureDesc.MipLevels        = 1;
    textureDesc.ArraySize        = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage            = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags        = 0; // no necesitás bind si solo la vas a compartir/copiar
    textureDesc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    if (device->CreateTexture2D(&textureDesc,nullptr,&frameTexture) != S_OK)
    {
        stderr_logger.log("No se pudo crear la textura de destino");
        return false;
    }
    //Castear la textura de destino al resource especifico para copiar

    if (frameTexture->QueryInterface(__uuidof( ID3D11Resource), (void**)&dstResource) != S_OK)
    {
        stderr_logger.log("No se pudo obtener el destiny resource");
        dstResource = nullptr;
        return false;
    }
    width = outputDesc.ModeDesc.Width;
    height = outputDesc.ModeDesc.Height;
    format = outputDesc.ModeDesc.Format;

    stdout_logger.log("Format: ", static_cast<int>(format));
    dstResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex);
   return  true;
}

bool DesktopDuplicatorManager::createWindowsHandler()
{
    if (!dstResource)
    {
        stderr_logger.log("createDestinyResource() should be called before.");
        return false;
    }
    IDXGIResource1* dstResource1 = nullptr;

    if (dstResource->QueryInterface(__uuidof( IDXGIResource1), (void**)&dstResource1) != S_OK)
    {
        stderr_logger.log("No se pudo obtener el destiny resource1");

        return false;
    }
    //Obtener handle
    if (dstResource1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ  , nullptr,&this->handle) != S_OK)
    {
        stderr_logger.log("No se pudo obtener el handle");
        return false;
    }
    return true;

}



bool DesktopDuplicatorManager::selectDuplicationOuput()
{
    if (!device)
    {
        stderr_logger.log("initializeID3D11() should be called before");
        return false;
    }
    // Create a DXGIFactory object.
    IDXGIFactory* pFactory = nullptr;
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&pFactory)))
    {
        stderr_logger.log("Failed to create factory");
        return false;
    }
    //Obtainig adapters
    IDXGIAdapter * pAdapter;
    std::vector<IDXGIAdapter*> vAdapters;
    for ( UINT i = 0;
          pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
          ++i )
    {
        vAdapters.push_back(pAdapter);
    }
    //Selecting adapter
    if (vAdapters.empty())
    {
        stderr_logger.log("Not available adapters");
        return false;
    }
    pAdapter = *vAdapters.data();

    //Obtaining outputs
    UINT i = 0;
    IDXGIOutput * pOutput;
    std::vector<IDXGIOutput*> vOutputs;
    while(pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
    {
        vOutputs.push_back(pOutput);
        ++i;
    }
    //Selecting outputs
    if (vOutputs.empty())
    {
        stderr_logger.log("Not available outputs");
        return false;
    }
    pOutput = *vOutputs.data(); //Just use first output (probabily the only one);
    IDXGIOutput1* output1 = nullptr;
    if (FAILED( pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1)) || output1 == nullptr)
    {
        stderr_logger.log("No se pudo obtener IDXGIOutput1");
        output1 = nullptr;
        return false;
    }


    if (FAILED(output1->DuplicateOutput(device,&outputDuplication)))
    {
        stderr_logger.log("Error obtaining duplicated output");
        outputDuplication = nullptr;
        return false;
    }


    return true;
}

bool DesktopDuplicatorManager::initializeID3D11()
{
    D3D_FEATURE_LEVEL featureLevel;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    //flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context);

#ifdef _DEBUG
  /*  if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
        stderr_logger.log("(D3D11) Debug layer no disponible, reintentando sin el flag. "
                     "Instala 'Graphics Tools' (Configuracion > Caracteristicas opcionales) para debug.");
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
            nullptr, 0, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);*/
    //}
#endif

    if (FAILED(hr)) {
        stderr_logger.log("Failed to initialize d3d11: 0x", std::hex, hr, std::dec);
        device = nullptr;
        return false;
    }

#ifdef _DEBUG
    ID3D11Debug* d3dDebug = nullptr;
    if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
    {
        ID3D11InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue)))
        {
            // NO usar SetBreakOnSeverity: eso es lo que probablemente
            // causaba el crash "Unknown signal" (rompe en un breakpoint
            // que MinGW/gdb no sabe interpretar). Solo vamos a imprimir.
            infoQueue->Release();
        }
        d3dDebug->Release();
    }
#endif
    return true;
}

VkFormat DesktopDuplicatorManager::dxgiToVulkanFormat(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat) {
    case DXGI_FORMAT_B8G8R8A8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return VK_FORMAT_R8G8B8A8_SRGB;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:    return VK_FORMAT_R16G16B16A16_SFLOAT;
    case DXGI_FORMAT_R10G10B10A2_UNORM:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    default:
        stderr_logger.log("(PIPELINE) Formato DXGI no soportado: ", static_cast<int>(dxgiFormat));
        return VK_FORMAT_UNDEFINED;
    }
}
