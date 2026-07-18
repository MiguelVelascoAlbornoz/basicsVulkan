
#include <imGUI/imgui_impl_sdl3.h>
#include "../App/App.h"
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
            switch (SDL_Keycode key = event.key.key) {
                case SDLK_ESCAPE:
                    acosh(key);
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
    const bool* keys = SDL_GetKeyboardState(nullptr);

    float speed = .001f;
    const vec3* cameraPosition = mainCamera.getPosition();
    mat3 cameraWorldMatrix = mainCamera.getWorldMatrix();
    bool updatePosition = false;
    vec3 newPosition = *cameraPosition;
    if (keys[SDL_SCANCODE_SPACE])
    {
        newPosition = newPosition + cameraWorldMatrix[0]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    } else if (keys[SDL_SCANCODE_W]) {
        newPosition = newPosition + cameraWorldMatrix[1]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    } else if (keys[SDL_SCANCODE_S]) {
        newPosition = newPosition - cameraWorldMatrix[1]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    } else if (keys[SDL_SCANCODE_A]) {
        newPosition = newPosition - cameraWorldMatrix[2]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    } else if (keys[SDL_SCANCODE_D]) {
        newPosition = newPosition + cameraWorldMatrix[2]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    } else if (keys[SDL_SCANCODE_RSHIFT]) {
        newPosition = newPosition - cameraWorldMatrix[0]*speed*static_cast<float>( deltaTime);
        updatePosition = true;
    }
    if (updatePosition) {
        mainCamera.setPosition(newPosition);
        uniformBuffer->addIndexToQueue(VIEW_PROJECTION_MATRIX);
    }
}
