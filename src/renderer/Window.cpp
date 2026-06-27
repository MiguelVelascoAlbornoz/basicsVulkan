/**
 * @file Window.cpp
 * @brief Window class implementation.
 * @author Miguel Velasco
 */
#include "Window.h"
#include <imGUI/imgui_impl_sdl3.h>
#include <iostream>
#include <SDL3/SDL_vulkan.h>


Window::Window(int width= 640, int height = 480) : width(width), height(height){
    
    SDL_SetAppMetadata(PROJECT_NAME, PROJECT_VERSION, "com.example.renderer-clear");
  
    /**< Initialize SDL. */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        error = true;
        return;
    }
    /**< Create window and renderer. */
    window = SDL_CreateWindow(PROJECT_NAME, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    
    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        error = true;
        return;
    }

    #ifdef _DEBUG
    std::cout << "Window created successfully." << std::endl;

    #endif

}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_Quit(); 
}

void Window::manageEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
       // ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
          setState(EventType::QUIT, true);
        } else {
          setState(EventType::QUIT, false);
        }

    }
}
