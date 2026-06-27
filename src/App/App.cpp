
/**
 * @file App.cpp
 * @brief App class implementation.
 * @author Miguel Velasco
 */
#include "App.h"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#include <iostream>



App::App() {
    window = new Window(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (window->isError()) {
        std::cerr << "Failed to initialize window." << std::endl;
        return;
    }
    renderer = new Renderer(window);
    if (renderer->error) {
        std::cerr << "Failed to initialize renderer." << std::endl;
        return;
    }
    
    menuManager = new MenuManager(window);

    executionLoop();
}

void App::manageEvents() {
    window->manageEvents();
    if (window->getEventState(Window::EventType::QUIT)) {
        runnig = false;
    }
}

void App::executionLoop() {
    
    while (runnig) {
        manageEvents();
        renderer->update(menuManager->currentMenu);
    }
}
/*
    MenuManager -> contem todos os menus e sabe qual menu renderizar
    Um menu é apenas uma função que se da ao renderer no momento em que ele vaja renderizar o GUI
    renderer -> ao renderizar da se um pointer a um menu manager, o menu manager tem uma variavel que indica qual menu deve ser renderizado, o renderer chama a função do menu manager para renderizar o menu atual
*/