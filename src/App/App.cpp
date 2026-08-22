
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#include <imGUI/imgui_impl_sdl3.h>
#include "../Renderer/Renderer.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Window.h"
#include "../Scene/Model.h"
#include "../Renderer/VulkanDevice.h"
#include "../Renderer/UniformBuffer.h"
#include "../Registry/Images.h"
#include "../Registry/ComputePipelines.h"
#include "../Renderer/ComputePipeline.h"
#include "../renderer/Image.h"

App::App(const std::function<void(App*)>& registryCallback) {
    FFT_N = 512;
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

    VkCommandBuffer cmd =renderer->getVulkanDevice()->beginSingleTimeCommands();

    ComputePipeline* computePipeline = nullptr;
    //LIMPIAR LA IMAGEM
    Images::images[IFFT_H0_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkImageSubresourceRange range= {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    VkClearColorValue clearColor = {0.0f,0.0f,0.0f,0.0f};
    vkCmdClearColorImage(cmd,Images::images[IFFT_H0_IMAGE_ID]->getImage(),VK_IMAGE_LAYOUT_GENERAL, &clearColor,1,&range);

    //Crear las ondas
    computePipeline = ComputePipelines::computePipelines[IFFT_SET_WAVES_COMPUTE_PIPELINE_ID];
    Images::images[IFFT_H0_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TRANSFER_BIT ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    computePipeline->bind(cmd);
    ComputePipeline::dispatch(cmd,
        1,  // grupos en X, redondeando hacia arriba
        1, // grupos en Y
        1);


    renderer->getVulkanDevice()->endSingleTimeCommands(cmd);

    Pipelines::defaultPipeline->updateDescriptorSet();
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

            managePlayerMovement();
            if (!editorMode) manageCameraRotation(mouseMotion);

            {
                VkCommandBuffer cmd =renderer->getVulkanDevice()->beginSingleTimeCommands();
                    // Generar h(k,t=0) a partir de h0Image
    Images::ifftInImage->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TRANSFER_BIT ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_H0_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    ComputePipeline* updateSpectrum = ComputePipelines::getPipeline(IFFT_UPDATE_SPECTRUM_COMPUTE_PIPELINE_ID);
    updateSpectrum->bind(cmd);
    float initialTime = (cycleStartTimeNS/1e9)*0.1;
    updateSpectrum->pushConstants(cmd, &initialTime, sizeof(float));
    ComputePipeline::dispatch(cmd, FFT_N/16, FFT_N/16, 1); // local_size_x=16,y=16 en el shader


    //stage 0 -> FFT en columnas
    Images::images[IFFT_DISPLACEMENT_TEMP_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_DISPLACEMENT_TEMP_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_DISPLACEMENT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_DERIVATES_TEMP_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::ifftInImage->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    ComputePipeline* computePipeline= ComputePipelines::getPipeline(IFFT_COMPUTE_PIPELINE_ID);
    computePipeline->bind(cmd);
    ComputePipelines::IfftComputePushConstants push= {0};
    vkCmdPushConstants(cmd, computePipeline->getPipelineLayout(),VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    ComputePipeline::dispatch(cmd,FFT_N, 5,1);


    //STAGE 1 -> FFT en filas
    computePipeline= ComputePipelines::getPipeline(IFFT_COMPUTE_PIPELINE_ID);
    Images::images[IFFT_DISPLACEMENT_TEMP_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_DISPLACEMENT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_DERIVATES_TEMP_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  ,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::ifftInImage->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    Images::images[IFFT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    computePipeline->bind(cmd);
    push= {1};
    vkCmdPushConstants(cmd, computePipeline->getPipelineLayout(),VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    ComputePipeline::dispatch(cmd,FFT_N,  5, 1);

                Images::images[IFFT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
                Images::images[IFFT_DISPLACEMENT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

/*
    //CREAR EL PNG
    Images::images[IFFT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);

    renderer->getVulkanDevice()->endSingleTimeCommands(cmd);

    Images::images[IFFT_OUT_IMAGE_ID]->saveColorImageToPNG("out.png");

    cmd =renderer->getVulkanDevice()->beginSingleTimeCommands();

*/
                renderer->getVulkanDevice()->endSingleTimeCommands(cmd);

            }


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
