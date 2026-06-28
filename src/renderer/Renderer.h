/**
 * @file Renderer.h
 * @brief Renderer class declaration and all his features.
 * @author Miguel Velasco
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <Vulkan/vulkan.h>
#include "Window.h"
#include <functional>
#include "../Menu/Menu.h"

/**
 * @brief The Renderer class encapsulates the creation and management of an SDL renderer, including error management and GUI initialization.
 * It provides methods to initialize the renderer, manage errors, and set up the GUI.
 */
class Renderer {
    public:
        /**
         * @brief Constructor of the Renderer class.
         * @details Initializes the SDL renderer for the given window and ImGUI. If the renderer fails to initialize, it sets the error flag to true and prints an error message to the standard error stream.
         */
        Renderer(Window* window);
        /**
         * @brief Destructor of the Renderer class.
         * @details Destroys the SDL renderer to clean up resources when the Renderer object is destroyed.
         */
        ~Renderer();;
        bool error = false; /**< @brief Flag to indicate if there was an error during initialization. */
        SDL_Renderer* renderer;
        /**
         * @brief Updates the renderer, including clearing the screen, rendering the GUI, and presenting the rendered content. This method should be called in the main execution loop to continuously update the display.
         * @details Clears the renderer, calls the updateGUI() method to render the GUI elements, and then presents the rendered content on the screen.
         */
        void update(Menu* renderMenu);
        /**
         * @brief Updates the GUI by starting a new ImGui frame, calling the provided renderMenu function to render the current menu, and finalizing the ImGui frame for rendering. This method should be called within the update() method to ensure that the GUI is rendered correctly.
         * @details Starts a new ImGui frame for both SDL3 and SDL_Renderer3 backends, calls the provided renderMenu function to render the current menu, and then finalizes the ImGui frame and renders the draw data using the SDL_Renderer3 backend.
         */
        void updateGUI(Menu* menu);

        void updateGUI();
    private:
    /**< @brief Initializes Vulkan for rendering. 
     * @details This method sets up the Vulkan instance, surface, physical device, logical device, swapchain, render pass, and pipeline. If any of the initialization steps fail, it sets the error flag to true and prints an error message to the standard error stream.
    */
    void initVulkan(Window* window); 
     // Métodos de inicialización
     /**
      * @brief Sees which ones are the availables GPUs and picks the best one for the application.
      * @details This method enumerates the available physical devices (GPUs) in the system in DEBUG mode
      */
    bool pickPhysicalDevice();
    /**
     * @brief Creates a logical device from the selected physical device which is the comunication channel between the application and the GPU.
     * @details 3 main objectives: 
     * 1. Especifies which queues the application will use for rendering and presentation.
     * 2. Especifies the features and extensions that the application requires from the physical device.
     * 3. Creates the logical device and retrieves the handles to the specified queues.
     */
    bool createLogicalDevice();
    /**
     * @brief Creates a swapchain for the selected physical device and logical device, which is a series of images that are presented to the screen in a loop.
     * @details The steps are:
     *  1. Obtener las capacidades de la superficie
     *  2. Obtener los formatos soportados
     * 3. Obtener los modos de presentación
     * 4. Elegir:
     *    - formato (VK_FORMAT_B8G8R8A8_SRGB,   
     *   - present mode (FIFO, MAILBOX...)
     *   - tamaño (window->getWidth(), window->getHeight())
     * 5. Rellenar VkSwapchainCreateInfoKHR
     * 6. vkCreateSwapchainKHR()
     * 7. Obtener las imágenes del swapchain
     * 8. Crear un ImageView para cada imagen
     **/
    bool createSwapchain(Window* window);
    bool createRenderPass();
    bool createPipeline();

    // Handles de Vulkan
    VkInstance       instance = NULL;
    VkSurfaceKHR     surface = NULL;
    VkPhysicalDevice physicalDevice = NULL;
    VkDevice         device = NULL;
    VkQueue          graphicsQueue = NULL;
    VkQueue          presentQueue = NULL;
    uint32_t graphicsQueueFamilyIndex = 0xFFFFFFFF;
    uint32_t presentQueueFamilyIndex = 0xFFFFFFFF;
    VkSwapchainKHR             swapchain      = VK_NULL_HANDLE;
    std::vector<VkImage>       swapchainImages;
    VkFormat                   swapchainFormat;
    VkExtent2D                 swapchainExtent;
    VkRenderPass     renderPass = NULL;
    VkPipeline       pipeline = NULL;
};

#endif