
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
        std::vector vertices = {
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

    Scenes::turnOnScene(Scenes::renderTest);
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
    Uint64 timeAcc = 0;
    Uint64 lastCycleTimeNS = SDL_GetTicks(); // Tiempo del último frame en segundos




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
        Uint64  cycleMissingTime = minNsPerCycle-(cycleStartTimeNS - SDL_GetTicksNS());
        if (0 <  cycleMissingTime)
        {
            SDL_DelayNS(cycleMissingTime);
        }

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
