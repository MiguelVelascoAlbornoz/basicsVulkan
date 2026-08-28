
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"

#include <d3d11.h>

#include <imGUI/imgui_impl_sdl3.h>
#include <wrl/client.h>
#include <dxgi1_2.h>

#include "../Renderer/Renderer.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Window.h"
#include "../Scene/Model.h"
#include "../Renderer/VulkanDevice.h"
#include "../Renderer/UniformBuffer.h"
#include "../Registry/Images.h"



App::App(const std::function<void(App*)>& registryCallback) {
    //Initizialise window
    window = new Window();
    if (window->isError()) {
        std::cerr << "Failed to initialize window." << std::endl;
        return;
    }
    //Initialize renderer
    renderer = new Renderer(window);
    if (renderer->error) {
        std::cerr << "Failed to initialize renderer." << std::endl;
        return;
    }
    #ifdef _DEBUG
    std::cout << "Renderer initialized successfully." << std::endl;
    #endif

    player = new Player(0);



    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    if (D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0,
            D3D11_SDK_VERSION,
            &device,
            &featureLevel,
            &context) != S_OK)
    {
        std::cerr << "Failed to initialize d3d11." << std::endl;
        runnig = false;
        return;
    }

    //Obtaining output monitor
        // Create a DXGIFactory object.
    IDXGIFactory* pFactory = NULL;
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&pFactory)))
    {
        std::cerr << "Failed to create factory" << std::endl;
        runnig = false;
        return;
    }
        //Obtainig adapters
    IDXGIAdapter * pAdapter;
    std::vector <IDXGIAdapter*> vAdapters;
    for ( UINT i = 0;
          pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
          ++i )
    {
        vAdapters.push_back(pAdapter);
    }
        //Selecting adapter
    if (vAdapters.empty())
    {
        std::cerr << "Not available adapters" << std::endl;
        runnig = false;
        return;
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
        std::cerr << "Not available outputs" << std::endl;
        runnig = false;
        return;
    }
    pOutput = *vOutputs.data();
    IDXGIOutput1* output1 = nullptr;
    HRESULT hr = pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);

    if (FAILED(hr) || output1 == nullptr)
    {
        std::cerr << "No se pudo obtener IDXGIOutput1" << std::endl;
        runnig = false;
        return;
    }

    //Obtaining duplicated output
    IDXGIOutputDuplication *ppOutputDuplication = nullptr;
    output1->DuplicateOutput(device,&ppOutputDuplication);
    if (ppOutputDuplication == nullptr)
    {
        std::cerr << "Error obtaining duplicated output" << std::endl;
        runnig = false;
        return;
    }

    //Obtaining frame
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* frameResource = nullptr;
    if (ppOutputDuplication->AcquireNextFrame(1000,&frameInfo,&frameResource) != S_OK)
    {
        std::cerr << "Error obtaining frame" << std::endl;
        runnig = false;
        return;
    }
    ID3D11Texture2D* frameTexture;
    hr = frameResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frameTexture);
    if (hr != S_OK)
    {
        std::cerr << "No se pudo obtener la textura" << std::endl;
        runnig = false;
        return;
    }

    //Now copy de frameTexturo into a new Texture
        //Crear la textra de destino
    D3D11_TEXTURE2D_DESC textureDesc;
    frameTexture->GetDesc(&textureDesc);
    textureDesc.MiscFlags  = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX ;

    ID3D11Texture2D* dstTexture = nullptr;
    if (device->CreateTexture2D(&textureDesc,nullptr,&dstTexture) != S_OK)
    {
        std::cerr << "No se pudo crear la textura de destino" << std::endl;
        runnig = false;
        return;
    }
        //Castear la textura de destino al resource especifico para copiar
    ID3D11Resource* dstResource = nullptr;
    hr = dstTexture->QueryInterface(__uuidof( ID3D11Resource), (void**)&dstResource);
    if (hr != S_OK)
    {
        std::cerr << "No se pudo obtener el destiny resource" << std::endl;
        runnig = false;
        return;
    }
        //Castear la textura de origen a un un resource especifico para copiar
    ID3D11Resource* srcResource = nullptr;
    hr = frameTexture->QueryInterface(__uuidof( ID3D11Resource), (void**)&srcResource);
    if (hr != S_OK)
    {
        std::cerr << "No se pudo obtener el source resource" << std::endl;
        runnig = false;
        return;
    }
        //Obtener contexto del device para hacer la copia
    ID3D11DeviceContext* deviceContext = nullptr;
    device->GetImmediateContext(&deviceContext);

        //Hacer la copia
    IDXGIKeyedMutex* keyedMutex = nullptr;
    dstResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyedMutex);

    keyedMutex->AcquireSync(0, INFINITE); // key 0 = "yo escribo"
    deviceContext->CopyResource(dstResource, frameTexture);
    keyedMutex->ReleaseSync(1);           // key 1 = "listo, que lea Vulkan"

        //Castear el resource de destino para poder obtener el handle
    IDXGIResource1* dstResource1 = nullptr;
    hr = frameTexture->QueryInterface(__uuidof( IDXGIResource1), (void**)&dstResource1);
    if (hr != S_OK)
    {
        std::cerr << "No se pudo obtener el destiny resource1" << std::endl;
        runnig = false;
        return;
    }
        //Obtener handle
    if (dstResource1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ  , nullptr,&desktopImageHandle) != S_OK)
    {
        std::cerr << "No se pudo obtener el handle" << std::endl;
        runnig = false;
        return;
    }
    desktopHeight = textureDesc.Height;
    desktopWidth = textureDesc.Width;
    desktopFormat = textureDesc.Format;


    registryCallback(this);

    FrameBuffers::turnOnFBO(FrameBuffers::defaultFrameBuffer);
    //Finally execution loop
    executionLoop();




}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    Meshes::freeMeshes();
    delete player;
    Uniforms::freeUniforms();
    Images::freeImages();
    delete window;
    delete renderer;
    Menus::freeMenus();
    }



void App::executionLoop()
{

    Uint64 timeAcc = 0;
    Uint64 lastCycleTimeNS = SDL_GetTicksNS(); // Tiempo del último frame en segundos




    unsigned int maxTicksUntilOverflow = 5;

    Uint64 lastSecond = 0;
    Uint64 cyclesCounter = 0;
    Uint64 ticksCounter = 0;

    //unsigned int lastSecond = 0;
    //unsigned int cyclesCounter = 0;
    while (runnig) {
        auto minNsPerCycle = 1000000000/player->getPlayerCameraSettings()->maxCyclesPerSecond;
        auto minNSPerTick = (1000000000/player->getPlayerCameraSettings()->maxTicksPerSecond);

        //Prepare time variables
        cycleStartTimeNS = SDL_GetTicksNS(); // Convertir a segundos
        cycleDeltaTimeNS = cycleStartTimeNS - lastCycleTimeNS; //Tiempo entre el inicio del ciclo interior y el inicio de este ciclo
        lastCycleTimeNS = cycleStartTimeNS;
        timeAcc += cycleDeltaTimeNS;


        //Get evenys
        manageEvents();
        //Call update to logic
        unsigned int ticks = 0;

        while (timeAcc >= static_cast<Uint64>(minNSPerTick) && ticks < maxTicksUntilOverflow )
        {
            tickDeltaTimeNS = SDL_GetTicksNS()- tickStartTimeNS;
            tickStartTimeNS = SDL_GetTicksNS();

            timeAcc -= minNSPerTick;
            ticks++;
            ticksCounter++;
        }

        //Render GUI
        renderGUI();

       // + + + + +  VULKAN RENDER + + + + +
        Uniforms::cameraUniform->addIndexToQueue(Uniforms::CameraUBO::TIME);
        Uniforms::cameraUniform->clearQueue();

        renderer->update();
        // - - - - - VULKAN RENDER END - - - -

        //For debug count the number off ticks and cycles in a second
        uint64_t secondTimer = SDL_GetTicks();
        unsigned int currentSecond = secondTimer / 1000;
        if (currentSecond != lastSecond)
        {// this code happens every second
            const unsigned int numSeconds =currentSecond- lastSecond ;


            cyclesPerSecond = static_cast<int>(cyclesCounter / numSeconds);
            ticksPerSecond = static_cast<int>(ticksCounter / numSeconds);

            lastSecond = currentSecond;
            cyclesCounter = 0;
            ticksCounter = 0;
        }
        cyclesCounter++;

        //Garantir um maximo de ciclos por segundo
        int  cycleMissingTime = static_cast<int>(minNsPerCycle-(SDL_GetTicksNS() - cycleStartTimeNS));
        if (0 <  cycleMissingTime)
        {
            SDL_DelayNS(cycleMissingTime);
        }

    }
    //debugTestFBO(this);

}

void App::renderGUI() {
    // 1. Preparar frame de ImGui (antes de tocar el command buffer)
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();


    Menus::drawMenus();

    ImGui::Render();
}
