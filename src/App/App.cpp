
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#include <imGUI/imgui_impl_sdl3.h>
#include "../renderer/Camera.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Pipeline.h"
#include "../Renderer/Mesh/FrameBufferObject.h"
#include "../Scene/Model.h"

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


void debugTestFBO(App* app)
{
    VulkanDevice* device = app->renderer->getVulkanDevice();

    // Descriptor pool propio para no pelear con el pool de la app
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = 1;

    VkDescriptorPool testDescriptorPool;
    vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &testDescriptorPool);

    FrameBufferObject fbo(device, 800, 600);
    if (fbo.error) { std::cerr << "Error creando FBO de test." << std::endl; return; }

    Pipeline::PipelineConfig config;
    config.vertexAttributes  = { AttribType::VEC3, AttribType::VEC3 };
    config.pushConstantsSize = sizeof(Model::ModelUBO);

    Pipeline testPipeline(device, fbo.getRenderPass(), config, testDescriptorPool);
    if (testPipeline.error) { std::cerr << "Error creando pipeline de test." << std::endl; return; }

    VkDescriptorBufferInfo bufferInfo{};
    VkWriteDescriptorSet write = Uniforms::cameraUniform->getWriteDescriptor(testPipeline.descriptorSet, 0, bufferInfo);
    vkUpdateDescriptorSets(device->device, 1, &write, 0, nullptr);

    VkCommandBuffer cmd = device->beginSingleTimeCommands();

    fbo.beginRenderPass(cmd);
    testPipeline.bind(cmd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, testPipeline.getPipelineLayout(),
                             0, 1, &testPipeline.descriptorSet, 0, nullptr);

    Model cubeModel;
    cubeModel.mesh = Meshes::cubeMesh;
    cubeModel.setTranslation(vec3(0.0f));
    cubeModel.setRotation(vec3(0.4f, 0.6f, 0.0f));
    cubeModel.setScale(vec3(1.0f));
    cubeModel.draw(cmd, &testPipeline);

    fbo.endRenderPass(cmd);
    device->endSingleTimeCommands(cmd);

    fbo.saveColorImageToPNG("fbo_debug.png");
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
    debugTestFBO(this);

}

void App::renderGUI() {
    // 1. Preparar frame de ImGui (antes de tocar el command buffer)
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();


    Menus::drawMenus();

    ImGui::Render();
}
