
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"


#include <imGUI/imgui_impl_sdl3.h>
#include "../Registry/ImGuiFonts.h"
#include "DesktopDuplicatorManager.h"
#include "../Renderer/Renderer.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Window.h"
#include "../Scene/Model.h"
#include "../Renderer/VulkanDevice.h"
#include "../Renderer/UniformBuffer.h"
#include "../Registry/Images.h"
#include "../Renderer/Image.h"
#include "../Renderer/Pipeline.h"

void App::startServer()
{
    type = App::HOST;
    if (desktopDuplicatorManager)
    {
        std::cerr << "StartServer(): A server is already existing." << std::endl;
        runnig = false;
    }
    desktopDuplicatorManager = new DesktopDuplicatorManager();
    if (!desktopDuplicatorManager->createDesktopDuplicator())
    {
        runnig = false;
    }
    desktopImage =Image::importFromD3D11Handle(renderer->getVulkanDevice(),desktopDuplicatorManager->getHandle(),desktopDuplicatorManager->getWidth(),desktopDuplicatorManager->getHeight(),DesktopDuplicatorManager::dxgiToVulkanFormat(desktopDuplicatorManager->getFormat()));


    VkCommandBuffer cmd = renderer->getVulkanDevice()->beginSingleTimeCommands();
    desktopImage->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    renderer->getVulkanDevice()->endSingleTimeCommands(cmd);
    desktopImage->setKeyedMutexSync(/*acquireKey=*/1, /*releaseKey=*/0);
    renderer->setSharedCaptureImage(Images::images[DESKTOP_IMAGE_ID]);

    std::vector<ImageBinding> imageBindings= {
            {
                .image = desktopImage->getView(),
                .sampler = desktopImage->getSampler(),
                .layout = desktopImage->getCurrentLayout(),
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
            }
    };
    std::vector<UniformBinding> uniformBindings= {};
    Pipelines::defaultPipeline->updateDescriptorSet(uniformBindings,imageBindings);
}

void App::startClient()
{
}

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



    registryCallback(this);

    FrameBuffers::turnOnFBO(FrameBuffers::defaultFrameBuffer);

    Menus::openMenu(CHOOSE_APP_TYPE_MENU_ID);

    //Finally execution loop
    executionLoop();




}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    ImGuiFonts::freeFonts();
    delete desktopDuplicatorManager;
    Meshes::freeMeshes();
    delete player;
    Uniforms::freeUniforms();
    Images::freeImages();
    delete renderer;
    delete window;

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
        if (type == HOST)
        {
            desktopDuplicatorManager->writeDestinyResource();
        }


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
