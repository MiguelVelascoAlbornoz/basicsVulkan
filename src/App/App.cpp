
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#include <iostream>
#include <memory>



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
    } else {
        #ifdef _DEBUG
        std::cout << "Renderer initialized successfully." << std::endl;
        #endif
    }
    
    menuManager = new MenuManager(window);

    scene = new Scene();
        std::vector<float> vertices = {
        // Posición (x, y, z)   // Color (r, g, b)
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f, // Vértice 1: rojo
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f, // Vértice 2: verde
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f  // Vértice 3: azul
    };
    std::vector<uint32_t> indices = { 0, 1, 2 }; // Un solo triángulo
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>( renderer->getVulkanDevice(), vertices.data(), sizeof(float) * 6, 3, indices);
    scene->addMesh(mesh);

    std::vector<VertexLayout::INPUT_TYPES> inputs = {
        VertexLayout::INPUT_TYPES::VEC3, // Posición
        VertexLayout::INPUT_TYPES::FLOAT  // time
    };
    uniformBuffer = new UniformBuffer(renderer->getVulkanDevice(),inputs);
    uniformBuffer->setVec3(0, glm::vec3(1.0f, 0.0f, 1.0f)); // Establece el color rojo
    uniformBuffer->updateDescriptorSet(renderer->getVulkanDevice(), renderer->descriptorSet);
    executionLoop();
}

void App::manageEvents() {
    window->manageEvents();
    if (window->getEventState(Window::EventType::QUIT)) {
        runnig = false;
    }
}

App::~App()
{
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
        if (uniformBuffer) {
            delete uniformBuffer;
        }
        if (scene) {
            delete scene;
        }
        delete window;
        if (renderer) {
            delete renderer;
        }
       
        if (menuManager) {
            delete menuManager;
        }
   
    }

void App::executionLoop()
{
    float lastFrameTime = SDL_GetTicks() / 1000.0f; // Tiempo del último frame en segundos
    while (runnig) {
        currentTime = SDL_GetTicks() / 1000.0f; // Convertir a segundos
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        uniformBuffer->setFloat(1, currentTime); // Actualiza el tiempo en el uniform buffer

        manageEvents();
        renderer->update(scene, menuManager->currentMenu);

        deltaTime = SDL_GetTicks() / 1000.0f - currentTime; // Diferencia de tiempo desde el último frame
    }
}
/*
    MenuManager -> contem todos os menus e sabe qual menu renderizar
    Um menu é apenas uma função que se da ao renderer no momento em que ele vaja renderizar o GUI
    renderer -> ao renderizar da se um pointer a um menu manager, o menu manager tem uma variavel que indica qual menu deve ser renderizado, o renderer chama a função do menu manager para renderizar o menu atual
*/