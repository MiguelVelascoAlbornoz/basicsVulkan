
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#include <imGUI/imgui_impl_sdl3.h>
#include "../renderer/Camera.h"
#include "../Registry/Scenes.h"

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
    Meshes::registerMesh("test_mesh",mesh);

    player = new Player(0);





    registryCallback(this);
    scene->renderFunc = Scenes::renderAxis;

    //Finally execution loop
    executionLoop();


}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    Meshes::freeMeshes();
    delete player;
    Uniforms::freeUniforms();
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
        Uniforms::cameraUniform->addIndexToQueue(Uniforms::CameraUBO::TIME);
        Uniforms::cameraUniform->clearQueue();

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
