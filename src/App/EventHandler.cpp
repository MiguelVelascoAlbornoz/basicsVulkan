
#include <imGUI/imgui_impl_sdl3.h>
#include "App.h"
#include <iostream>


void App::manageEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
           
          runnig = false;
          continue;
        } 
        if (event.type == SDL_EVENT_KEY_DOWN) {
            SDL_Keycode key = event.key.key;
            switch (key) {
                case SDLK_ESCAPE:
                    runnig = false;
                    break;
                case SDLK_F11:
                    this->window->toggleFullscreen();
                    break;
                default:
                    break;
            }
        } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            std::cout << "Window close requested." << std::endl;
            runnig = false;
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            if (!renderer->onWindowResized(window)) {
                std::cerr << "Failed to handle window resize event." << std::endl;
                runnig = false;
            }
        }
    }
}
