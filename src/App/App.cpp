
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#include <iostream>
#include <memory>
#include <imGUI/imgui.h>
#include <imGUI/imgui_impl_sdl3.h>

#include "../renderer/Mesh/Meshes.h"
#include "../Menu/EditorMenu.h"
#include "../renderer/Camera.h"



App::App() {
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



    //Test things
    scene = new Scene();
        std::vector<float> vertices = {
        // Position (x, y, z)   // Color (r, g, b)
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f, // Vértice 1: rojo
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f, // Vértice 2: verde
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f  // Vértice 3: azul
    };
    std::vector<uint32_t> indices = { 0, 1, 2 }; // Un solo triangular
    Mesh* mesh = new Mesh( renderer->getVulkanDevice(), vertices.data(), sizeof(float) * 6, 3, indices);
    scene->addMesh(mesh);
    player = new Player(0);

    std::vector<std::pair<const void*,AttribType::INPUT_TYPES>> inputsMap = {

         {player->camera->getViewProjectionMatrix(),AttribType::MAT4},
         {&currentTimeInSeconds,AttribType::FLOAT}
    };
    uniformBuffer = new UniformBuffer(renderer->getVulkanDevice(),inputsMap);





    //Registers
    Menus::registerMenu(EDITOR_MENU,new EditorMenu(player,[this](){onPlayerRenderUpdate();}));

    uniformBuffer->updateDescriptorSet(renderer->getVulkanDevice(), renderer->getPipeline("test")->descriptorSet);
    uniformBuffer->updateDescriptorSet(renderer->getVulkanDevice(), renderer->getPipeline("lines")->descriptorSet);
    Meshes::registerMesh("test_mesh",mesh);

    scene->renderFunc = [this](VkCommandBuffer commandBuffer)
    {
        Pipeline* defaultPipeline = renderer->getPipeline("lines");
        defaultPipeline->bind(commandBuffer); // bind the graphics pipeline (shaders + fixed-function state);
        vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        defaultPipeline->getPipelineLayout(),
        0,
        1,
        &defaultPipeline->descriptorSet,
        0,
        nullptr
    );
        Meshes::meshes["test_mesh"]->bind(commandBuffer);
        Meshes::meshes["test_mesh"]->draw(commandBuffer);
    };
    //Finally execution loop
    executionLoop();


}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    Meshes::freeMeshes();
    delete player;
    delete uniformBuffer;
    delete scene;
    delete window;
    delete renderer;
    Menus::freeMenus();
    }

void App::executionLoop()
{
    Uint64 lastFrameTime = SDL_GetTicks(); // Tiempo del último frame en segundos
    while (runnig) {
        currentTime = SDL_GetTicks(); // Convertir a segundos
        currentTimeInSeconds = static_cast<float>(currentTime)/1000.0f;
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        manageEvents();

        //Render GUI
        renderGUI();

       // + + + + +  VULKAN RENDER + + + + +
        uniformBuffer->addIndexToQueue(TIME_UNIFORM);
        uniformBuffer->clearQueue();

        renderer->update(scene);
        // - - - - - VULKAN RENDER END - - - -

        deltaTime = SDL_GetTicks()  - currentTime; // Diferencia de tiempo desde el último frame
    }
}

void App::renderGUI() {
    // 1. Preparar frame de ImGui (antes de tocar el command buffer)
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();


    Menus::drawMenus();

    ImGui::Render();
}
