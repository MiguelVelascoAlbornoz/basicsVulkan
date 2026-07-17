/**
 * @file Window.cpp
 * @brief Window class implementation.
 * @author Miguel Velasco
 */
#include "Window.h"
#include <imGUI/imgui_impl_sdl3.h>
#include <iostream>
#include <SDL3/SDL_vulkan.h>

void Window::toggleFullscreen()
{
    if (fullscreen) {
        width /= DEFAULT_REDUCTION_FACTOR;
        height /= DEFAULT_REDUCTION_FACTOR;
        SDL_SetWindowFullscreen(window, false);
        SDL_SetWindowSize(window, width, height);
        SDL_SetWindowPosition(window,
                      SDL_WINDOWPOS_CENTERED,
                      SDL_WINDOWPOS_CENTERED);
    } else {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
        width = mode->w;
        height = mode->h;
   
        SDL_SetWindowFullscreen(window, true);
        SDL_SyncWindow(window);
    }
    
    fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::inicializeSDL()
{
    SDL_SetAppMetadata(PROJECT_NAME, PROJECT_VERSION, "com.example.renderer-clear");
  
    /**< Initialize SDL. */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        error = true;
    }
}

Window::Window(){
    inicializeSDL();

    SDL_DisplayID display = SDL_GetPrimaryDisplay();

    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);

    width = mode->w;
    height = mode->h;
    fullscreen = false;
    if (!fullscreen) {
        width /= DEFAULT_REDUCTION_FACTOR;
        height /= DEFAULT_REDUCTION_FACTOR;
    }
    createWindow(width, height);
}
Window::Window(int width, int height) {
    inicializeSDL();
    createWindow(width, height);
}
void Window::createWindow(int width, int height) {


    /**< Create window and renderer. */
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
    if (fullscreen) {
        flags = flags | SDL_WINDOW_FULLSCREEN;
    }
    window = SDL_CreateWindow(PROJECT_NAME, width, height,  flags);
    
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
