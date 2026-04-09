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

#include <vulkan/vulkan.h>
#include <iostream>

bool Window::testVulkan() {
    VkInstance instance;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_SUCCESS) {
        std::cout << "Vulkan está disponible!\n";
        vkDestroyInstance(instance, nullptr);
        return true;
    } else {
        std::cout << "Vulkan NO está disponible. Código: " << result << "\n";
        return false;
    }
}