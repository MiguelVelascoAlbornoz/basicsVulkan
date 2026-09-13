//
// Created by migue on 29/08/2026.
//

#ifndef BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H
#define BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H

#include <d3d11.h>
#include <windows.h>

#include <vulkan/vulkan.h>
struct ID3D11DeviceContext;
struct ID3D11Device;
struct IDXGIOutputDuplication;
struct ID3D11Resource;
struct IDXGIKeyedMutex;
struct ID3D11Texture2D;
class DesktopDuplicatorManager
{

public:
    static VkFormat dxgiToVulkanFormat(DXGI_FORMAT dxgiFormat);
    bool createDesktopDuplicator();
    ~DesktopDuplicatorManager();

    [[nodiscard]] bool writeDestinyResource() const;
    ID3D11Device* device = nullptr;
    [[nodiscard]] HANDLE getHandle() const
    {
        return handle;
    };
    [[nodiscard]] int getWidth() const
    {
        return width;
    };
    [[nodiscard]] int getHeight() const
    {
        return height;
    };
    [[nodiscard]] DXGI_FORMAT getFormat() const
    {
        return format;
    };
    /** @brief Textura dedicada al encoder, NO compartida con Vulkan.
     *  @details Se llena dentro del mismo bloque protegido por el keyed mutex
     *  que llena dstResource, así que su contenido siempre es consistente
     *  sin necesitar que el consumidor (encoder) participe del protocolo
     *  de acquire/release del keyed mutex. */
    [[nodiscard]] ID3D11Texture2D* getEncoderCaptureTexture() const { return encoderCaptureTexture; }

    ID3D11Resource* dstResource = nullptr;
    ID3D11DeviceContext* context = nullptr;
private:
    ID3D11Texture2D* frameTexture = nullptr;
    ID3D11Texture2D* encoderCaptureTexture = nullptr;
    bool initializeID3D11();
    bool selectDuplicationOuput();
    bool createDestinyResource();
    bool createEncoderCaptureTexture();
    bool createWindowsHandler();
    int width = 0, height = 0;
    HANDLE handle = nullptr;
    DXGI_FORMAT format;
    IDXGIKeyedMutex* keyedMutex = nullptr;


    IDXGIOutputDuplication* outputDuplication = nullptr;


};


#endif //BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H