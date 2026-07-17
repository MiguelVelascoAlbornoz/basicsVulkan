
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#include <iostream>
#include <memory>
#include "../renderer/Camera.h"



Camera mainCamera;
vec3 triangueCol(1.0f,0.0f,1.0f);
App::App() {
    window = new Window();
    if (window->isError()) {
        std::cerr << "Failed to initialize window." << std::endl;
        return;
    }
    renderer = new Renderer(window);
    if (renderer->error) {
        std::cerr << "Failed to initialize renderer." << std::endl;
        return;
    }
    #ifdef _DEBUG
    std::cout << "Renderer initialized successfully." << std::endl;
    #endif

    
    menuManager = new MenuManager(window);

    scene = new Scene();
        std::vector<float> vertices = {
        // Position (x, y, z)   // Color (r, g, b)
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f, // Vértice 1: rojo
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f, // Vértice 2: verde
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f  // Vértice 3: azul
    };
    std::vector<uint32_t> indices = { 0, 1, 2 }; // Un solo triangular
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>( renderer->getVulkanDevice(), vertices.data(), sizeof(float) * 6, 3, indices);
    scene->addMesh(mesh);


    std::vector<std::pair<void*,AttribType::INPUT_TYPES>> inputsMap = {
        {&triangueCol,AttribType::VEC3}, {&currentTime,AttribType::FLOAT}
    };
    uniformBuffer = new UniformBuffer(renderer->getVulkanDevice(),inputsMap);


    uniformBuffer->updateDescriptorSet(renderer->getVulkanDevice(), renderer->descriptorSet);
  //  int flag = 1;
  //  for (int i = 0; i < cameraToShader.size(); ++i) {
      //  if (mainCamera.isDirty(static_cast<Camera::DirtyFlags>(flag))) {
            //mat4 viewProjectionMatrix = mainCamera.getViewProjectionMatrix();
            //uniformBuffer->setRaw(2 + i, &viewProjectionMatrix, sizeof(glm::mat4));
     //   }
      //  flag = flag << 1;
   // }
    executionLoop();
}



App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    delete uniformBuffer;
    delete scene;
    delete window;
    delete renderer;
    delete menuManager;
    }

void App::executionLoop()
{
    Uint64 lastFrameTime = SDL_GetTicks(); // Tiempo del último frame en segundos
    while (runnig) {
        currentTime = SDL_GetTicks(); // Convertir a segundos
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        manageEvents();
        //Try update camera fields
        uniformBuffer->addIndexToQueue(1);
        uniformBuffer->clearQueue();

        renderer->update(scene, menuManager->currentMenu);

        deltaTime = SDL_GetTicks()  - currentTime; // Diferencia de tiempo desde el último frame
    }
}
/*
    MenuManager -> contem todos os menus e sabe qual menu renderizar
    Um menu é apenas uma função que se da ao renderer no momento em que ele vaja renderizar o GUI
    renderer -> ao renderizar da se um pointer a um menu manager, o menu manager tem uma variavel que indica qual menu deve ser renderizado, o renderer chama a função do menu manager para renderizar o menu atual
*/