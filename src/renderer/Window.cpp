/**
 * @file Window.cpp
 * @brief Window class implementation.
 * @author Miguel Velasco
 */

#include "Window.h"

Window::Window() {
    SDL_SetAppMetadata(PROJECT_NAME, PROJECT_VERSION, "com.example.renderer-clear");
  
    /**< Initialize SDL. */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        error = SDL_APP_FAILURE;
    }
    /**< Create window and renderer. */
    if (!SDL_CreateWindowAndRenderer(PROJECT_NAME, 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        error = SDL_APP_FAILURE;
    }
    
    /**< Set logical presentation. */
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    error = SDL_APP_SUCCESS;
}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit(); 
}
