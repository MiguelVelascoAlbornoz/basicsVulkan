/**
 * @file Renderer.h
 * @brief Renderer class declaration and all his features.
 * @author Miguel Velasco
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <functional>

#include "Pipeline.h"
#include "SwapChain.h"
#include "../Scene/Scene.h"
#include "../App/Utilitys.h"
#include "vulkan/vulkan.hpp"
/**
 * @brief The Renderer class encapsulates the creation and management of an SDL renderer, including error management and GUI initialization.
 * It provides methods to initialize the renderer, manage errors, and set up the GUI.
 */
class Renderer {
    public:
        Pipeline* defaultPipeline;
        Pipeline* linesPipeline;
        /**
         * @brief Constructor of the Renderer class.
         * @details Initializes the SDL renderer for the given window and ImGUI. If the renderer fails to initialize, it sets the error flag to true and prints an error message to the standard error stream.
         */
        explicit Renderer(Window* window);
        /**
         * @brief Destructor of the Renderer class.
         * @details Destroys the SDL renderer to clean up resources when the Renderer object is destroyed.
         */
        ~Renderer();
        bool error = false; /**< @brief Flag to indicate if there was an error during initialization. */
        SDL_Renderer* renderer;
        [[nodiscard]] VulkanDevice* getVulkanDevice() const { return vulkanDevice; } /**< @brief Get the Vulkan device used by the renderer. */
        void recordCommandBuffer(Scene* scene,VkCommandBuffer commandBuffer, uint32_t imageIndex);
        /**
         * @brief Updates the renderer, including clearing the screen, rendering the GUI, and presenting the rendered content. This method should be called in the main execution loop to continuously update the display.
         * @details Clears the renderer, calls the updateGUI() method to render the GUI elements, and then presents the rendered content on the screen.
         */
        void update(Scene* scene);


        bool onWindowResized(Window* window);

        Pipeline* getPipeline(const std::string& pipelineID) {
            return pipelines[pipelineID];
        }
        Pipeline* registerPipelines(const std::string &pipelineID, Pipeline::PipelineConfig* config) {
            Pipeline* newPipeline = new Pipeline(vulkanDevice,renderPass,*config);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &newPipeline->descriptorSetLayout;



            vkAllocateDescriptorSets(
              vulkanDevice->device,
              &allocInfo,
              &newPipeline->descriptorSet
            );
            if (newPipeline->error) {
                this->error = true;
                return NULL;
            }
            return registerObject(pipelineID,newPipeline,pipelines);
        }
        void freePipelines() {
            for (const auto& pipeline : pipelines) {
                delete pipeline.second;
            }
        }

private:
    std::unordered_map<std::string, Pipeline*> pipelines; /**< @brief Map to store menu rendering functions. */

    void initGUI(const Window *window);
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


    /**
     * @brief Creates the render pass, which describes the attachments (color, depth, etc.) used during rendering and how they are handled across subpasses.
     * 
     * @details Defines:
     * 1. A color attachment description (format taken from swapchainFormat, load/store ops, initial/final layout).
     * 2. A single subpass that references the color attachment.
     * 3. A subpass dependency to synchronize the implicit layout transition with the swapchain image acquisition.
     * The resulting VkRenderPass is required by createFramebuffers() and createPipeline().
     */
    bool createRenderPass(VkSurfaceFormatKHR* chosenFormat);



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


// ==================== Pipeline gráfico ====================

/// @brief Render pass, describe los attachments y su manejo a lo largo de las subpasses.
/** @note RenderPass define:
 * 
- Cuántos attachments hay (color, depth, etc.)

- Sus formatos

- Cómo se cargan/guardan

- Los subpasses
 */
VkRenderPass renderPass = VK_NULL_HANDLE;

VulkanDevice* vulkanDevice = nullptr;

SwapChain* swapChain = nullptr;
// ==================== Comandos ====================


/// @brief Command buffers de dibujo, uno por cada frame en vuelo (ver MAX_FRAMES_IN_FLIGHT).
std::vector<VkCommandBuffer> commandBuffers;

// ==================== Sincronización ====================

/// @brief Número máximo de frames procesándose en paralelo (CPU grabando mientras GPU dibuja otro).
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

/// @brief Semáforos señalados cuando la imagen de la swapchain está lista para dibujar en ella. Uno por frame en vuelo.
std::vector<VkSemaphore> imageAvailableSemaphores = {};

/// @brief Semáforos señalados cuando terminó de dibujarse el frame, antes de presentar. Uno por frame en vuelo.
std::vector<VkSemaphore> renderFinishedSemaphores = {};

std::vector<VkFence> inFlightFences = {}; /**< @brief Vallas (fences) que bloquean la CPU hasta que la GPU terminó de procesar un frame. Uno por frame en vuelo. */

/// @brief Índice del frame actual dentro del ciclo de MAX_FRAMES_IN_FLIGHT.
uint32_t currentFrame = 0;

/// @brief Descriptor pool usado por ImGui para asignar sus descriptor sets.
VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE; /**< @brief Descriptor pool usado por ImGui para asignar sus descriptor sets. */
VkDescriptorPool descriptorPool = VK_NULL_HANDLE; /**< @brief Descriptor pool usado por la aplicación para asignar sus descriptor sets. */

};

#endif