/**
 * @file Renderer.h
 * @brief Renderer class declaration and all his features.
 * @author Miguel Velasco
 */

#ifndef RENDERER_H
#define RENDERER_H


#include <functional>
#include "../Menu/Menu.h"
#include "Pipeline.h"
#include "SwapChain.h"
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
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
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
    void initGUI(Window *window);
    /**< @brief Initializes Vulkan for rendering.
     * @details This method sets up the Vulkan instance, surface, physical device, logical device, swapchain, render pass, and pipeline. If any of the initialization steps fail, it sets the error flag to true and prints an error message to the standard error stream.
     */
    void initVulkan(Window *window);
    /**
     * @brief Initializes the Vulkan instance, which is the entry point to the Vulkan API.
     * @details This method creates a Vulkan instance with the required extensions and validation layers. If the instance creation fails, it sets the error flag to true and prints an error message to the standard error stream.
     * @return true if the Vulkan instance was created successfully, false otherwise.
     */
    bool createVulkanInstance();

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
     * @brief Creates the render pass, which describes the attachments (color, depth, etc.) used during rendering and how they are handled across subpasses.
     * 
     * @details Defines:
     * 1. A color attachment description (format taken from swapchainFormat, load/store ops, initial/final layout).
     * 2. A single subpass that references the color attachment.
     * 3. A subpass dependency to synchronize the implicit layout transition with the swapchain image acquisition.
     * The resulting VkRenderPass is required by createFramebuffers() and createPipeline().
     */
    bool createRenderPass();

    /**
     * @brief Wraps SPIR-V bytecode into a VkShaderModule so it can be used in a pipeline shader stage.
     * @param code The raw SPIR-V bytecode, read from a compiled .spv file.
     * @details Vulkan does not compile GLSL; this only packages bytecode that was already compiled offline (e.g. with glslc) into a Vulkan object. codeSize must be a multiple of 4, since SPIR-V is a stream of 32-bit words.
     * @return A valid VkShaderModule handle. Throws std::runtime_error if creation fails.
     */
    VkShaderModule createShaderModule(const std::vector<char> &code);

    /**
     * @brief Creates the pipeline layout, which describes the descriptor set layouts and push constant ranges accessible to the shaders.
     * @details Currently empty (no descriptor sets, no push constants), but required before createPipeline() since VkGraphicsPipelineCreateInfo needs a valid VkPipelineLayout.
     */
    bool createPipelineLayout();






    /**
     * @brief Creates the command pool from which command buffers are allocated.
     * @details Command buffers allocated from this pool are submitted to the graphics queue family (graphicsFamilyIndex). Uses VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT to allow individual command buffers to be reset/re-recorded each frame.
     */
    bool createCommandPool();

    /**
     * @brief Allocates the primary command buffers used to record draw commands, one per frame in flight.
     * @details Allocated from commandPool with VK_COMMAND_BUFFER_LEVEL_PRIMARY (submittable directly to a queue). The number allocated is MAX_FRAMES_IN_FLIGHT.
     */
    bool createCommandBuffers();

    /**
     * @brief Creates the synchronization primitives needed to coordinate CPU/GPU and GPU/GPU work per frame in flight.
     * @details Creates, per frame in flight:
     * - imageAvailableSemaphore: signaled when the swapchain image is ready to be rendered into.
     * - renderFinishedSemaphore: signaled when rendering has finished, before presenting.
     * - inFlightFence: blocks the CPU from reusing a command buffer still in use by the GPU. Created in the signaled state so the first frame doesn't block indefinitely.
     */
    bool createSyncObjects();
    // ==================== Handles de Vulkan ====================

/// @brief Instancia de Vulkan, punto de entrada a la API.
VkInstance instance = VK_NULL_HANDLE;

/// @brief Superficie de presentación, vincula Vulkan con la ventana de SDL.
VkSurfaceKHR surface = VK_NULL_HANDLE;

/// @brief GPU física seleccionada por pickPhysicalDevice().
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

/// @brief Dispositivo lógico, canal de comunicación con la GPU seleccionada.
VkDevice device = VK_NULL_HANDLE;

/// @brief Cola de comandos de graphics, donde se envían los command buffers de dibujo.
VkQueue graphicsQueue = VK_NULL_HANDLE;

/// @brief Cola de presentación, usada para mostrar imágenes en la superficie.
VkQueue presentQueue = VK_NULL_HANDLE;

/// @brief Índice de la queue family que soporta operaciones de graphics.
uint32_t graphicsQueueFamilyIndex = 0xFFFFFFFF;

/// @brief Índice de la queue family que soporta presentación en la superficie.
uint32_t presentQueueFamilyIndex = 0xFFFFFFFF;


// ==================== Pipeline gráfico ====================

/// @brief Render pass, describe los attachments y su manejo a lo largo de las subpasses.
VkRenderPass renderPass = VK_NULL_HANDLE;

SwapChain* swapChain;
Pipeline* pipeline; /**< @brief Pipeline gráfico: shaders + estado de rasterización/blending/etc. empaquetados como un solo objeto inmutable. */
// ==================== Comandos ====================

/// @brief Pool desde el cual se asignan los command buffers.
VkCommandPool commandPool = VK_NULL_HANDLE;

/// @brief Command buffers de dibujo, uno por cada frame en vuelo (ver MAX_FRAMES_IN_FLIGHT).
std::vector<VkCommandBuffer> commandBuffers;

// ==================== Sincronización ====================

/// @brief Número máximo de frames procesándose en paralelo (CPU grabando mientras GPU dibuja otro).
static const int MAX_FRAMES_IN_FLIGHT = 2;

/// @brief Semáforos señalados cuando la imagen de la swapchain está lista para dibujar en ella. Uno por frame en vuelo.
std::vector<VkSemaphore> imageAvailableSemaphores;

/// @brief Semáforos señalados cuando terminó de dibujarse el frame, antes de presentar. Uno por frame en vuelo.
std::vector<VkSemaphore> renderFinishedSemaphores;

std::vector<VkFence> inFlightFences; /**< @brief Vallas (fences) que bloquean la CPU hasta que la GPU terminó de procesar un frame. Uno por frame en vuelo. */

/// @brief Índice del frame actual dentro del ciclo de MAX_FRAMES_IN_FLIGHT.
uint32_t currentFrame = 0;

/// @brief Descriptor pool usado por ImGui para asignar sus descriptor sets.
VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE; /**< @brief Descriptor pool usado por ImGui para asignar sus descriptor sets. */
};

#endif