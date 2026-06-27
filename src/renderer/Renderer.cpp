/**
 * @file Renderer.cpp
 * @brief Renderer class implementation.
 * @author Miguel Velasco
 */
#include "Renderer.h"
#include <iostream>
#include <imGUI\imgui.h>
#include <imGUI\imgui_impl_sdl3.h>
#include <imGUI\imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL_vulkan.h>


/**
 * @brief Initializes the GUI using ImGui with SDL3 and SDL_Renderer3 backends.
 * @details This function sets up the ImGui context, configures the IO settings, If any of the initialization steps fail, it sets the error flag in the Renderer class to true and prints an error message to the standard error stream.
 */
void initGUI(Window* window, Renderer* renderer)
{
    ImGuiContext* ctx = ImGui::CreateContext();
    if (!ctx) {
        std::cerr << "Error: No se pudo crear el contexto de ImGui." << std::endl;
        renderer->error = true;
        return;
    }   

    ImGui::StyleColorsDark();

 
    bool ok1 = ImGui_ImplSDL3_InitForSDLRenderer(window->window, renderer->renderer);
    bool ok2 = ImGui_ImplSDLRenderer3_Init(renderer->renderer);;

    if (!ok1 || !ok2) {
        std::cerr << "Error: ImGui no se inicializó correctamente." << std::endl;
        renderer->error = true;
    } 
    #ifdef _DEBUG 
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    std::cout << "ImGui version: " << ImGui::GetVersion() << std::endl;
    std::cout << "Backend Renderer: " << (io.BackendRendererName ? io.BackendRendererName : "null") << std::endl;
    std::cout << "Backend Platform: " << (io.BackendPlatformName ? io.BackendPlatformName : "null") << std::endl;
    std::cout << "Display size: " << io.DisplaySize.x << " x " << io.DisplaySize.y << std::endl;
    
    #endif
}

void Renderer::initVulkan(Window* window)  {

    /**
     * Create Vulkan instance
     */
        // Extensions necesarias para SDL3
    Uint32 extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    #ifdef _DEBUG
        std::cout << "(VULKAN) Extensions:" << std::endl;
    for (unsigned int i = 0; i < extensionCount; ++i) {
        std::cout << " - " << extensions[i] << std::endl;
    }
    #endif

    VkApplicationInfo appInfo{};
    appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = PROJECT_NAME;
    //appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "BasicsVulkan";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_SUCCESS) {
        #ifdef _DEBUG
        std::cout << "(VULKAN) Vulkan está disponible!\n";
        #endif
    } else {
        std::cout << "(VULKAN) Vulkan no está disponible. Código: " << result << "\n";
        error = true;
        return;
    }
    /**
     * Attach Vulkan surface to SDL window
     */
    if (!SDL_Vulkan_CreateSurface(window->window, instance, nullptr, &surface)) {
        std::cout << "(VULKAN) Failed to create surface: " << SDL_GetError() << "\n";
        error = true;
        return;
    } else {
        #ifndef _DEBUG
        std::cout << "(VULKAN) Vulkan surface created successfully.\n";
        #endif
    }
    if (!pickPhysicalDevice() || !createLogicalDevice() || !createSwapchain(window) || !createRenderPass() || !createPipeline()) {
        error = true;
    }

}

Renderer::Renderer(Window *window)
{
    initVulkan(window);
    //initGUI(window, this);

    
    #ifdef _DEBUG
    std::cout << "(SDL) Available render drivers:" << std::endl;
    int numDrivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < numDrivers; ++i) {
        const char* info = SDL_GetRenderDriver(i);
        std::cout << " - " << info << std::endl;
    }
    std::cout << "Renderer created successfully." << std::endl;
    #endif
}

Renderer::~Renderer()
{
    //ImGui
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
   
    vkDestroyDevice(device, nullptr);         // antes que la instancia
    vkDestroySurfaceKHR(instance, surface, nullptr); // antes que la instancia
    vkDestroyInstance(instance, nullptr);     // último
}

void Renderer::update(Menu* renderMenu)
{
    renderMenu->getId();
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // gris oscuro, opcional
    
    SDL_RenderClear(renderer);

    //Render normal things
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_FRect rect = {50, 60, 50, 50};
    SDL_RenderFillRect(renderer, &rect);


    //Render GUI
    //updateGUI(renderMenu);
 
    //Update window
    SDL_RenderPresent(renderer);
}

void Renderer::updateGUI(Menu* menu)
{
    /**
     * @brief Starts a new ImGui frame for both SDL3 and SDL_Renderer3 backends. This function should be called at the beginning of each frame before submitting any ImGui UI commands. It prepares the ImGui context for a new frame, allowing you to create and submit your ImGui UI elements for rendering.
     */
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    menu->render();

    /**
     * @brief Finalizes the ImGui frame and renders the draw data. This function should be called after submitting all your ImGui UI commands for the current frame. It ends the ImGui frame, finalizes the draw data, and then calls the rendering function of the SDL_Renderer3 backend to render the ImGui elements on the screen.
     */
    ImGui::Render();

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}
