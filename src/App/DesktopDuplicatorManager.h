//
// Created by migue on 29/08/2026.
//

#ifndef BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H
#define BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H
#include <d3d11.h>

struct ID3D11DeviceContext;
struct ID3D11Device;
struct IDXGIOutputDuplication;
struct ID3D11Resource;

class DesktopDuplicatorManager
{
    public:
    bool createDesktopDuplicator();

    bool writeDestinyResource();
    [[nodiscard]] const HANDLE* getHandle() const
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

private:
    bool initializeID3D11();
    bool selectDuplicationOuput();
    bool createDestinyResource();
    bool createWindowsHandler();
    int width = 0, height = 0;
    HANDLE* handle = nullptr;
    DXGI_FORMAT format;
    ID3D11DeviceContext* context = nullptr;
    ID3D11Device* device = nullptr;
    IDXGIOutputDuplication* outputDuplication = nullptr;
    ID3D11Resource* dstResource = nullptr;
 };


#endif //BASICSVULKAN_DESKTOPDUPLICATORMANAGER_H
