#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H


#include <imGUI\imgui_impl_vulkan.h>
#include <vector>
class VulkanDevice {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    ImGui_ImplVulkan_InitInfo getImGuiInfo(VkDescriptorPool& imguiDescriptorPool, VkInstance instance, int imageCount) const;
    bool error = false;
    // Métodos de inicialización
    /**
     * @brief Sees which ones are the availables GPUs and picks the best one for the application.
     * @details This method enumerates the available physical devices (GPUs) in the system in DEBUG mode
     */
    bool pickPhysicalDevice(VkInstance instance);
    /**
     * @brief Creates a logical device from the selected physical device which is the comunication channel between the application and the GPU.
     * @details 3 main objectives:
     * 1. Especifies which queues the application will use for rendering and presentation.
     * 2. Especifies the features and extensions that the application requires from the physical device.
     * 3. Creates the logical device and retrieves the handles to the specified queues.
     */
    bool createLogicalDevice(VkSurfaceKHR surface);


    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; } /**< @brief Get the handle to the selected physical device. */
    bool queueSubmit( const VkSubmitInfo* submitInfo, VkFence fence); /**< @brief Submits a command buffer to the specified queue for execution. */
    bool presentKHR(VkPresentInfoKHR *presentInfo);
        /**
     * @brief Creates the command pool from which command buffers are allocated.
     * @details Command buffers allocated from this pool are submitted to the graphics queue family (graphicsFamilyIndex). Uses VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT to allow individual command buffers to be reset/re-recorded each frame.
     */
    bool createCommandPool();
    /**
     * @brief Allocates the primary command buffers used to record draw commands, one per frame in flight.
     * @details Allocated from commandPool with VK_COMMAND_BUFFER_LEVEL_PRIMARY (submittable directly to a queue). The number allocated is MAX_FRAMES_IN_FLIGHT.
     */
    bool createCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkDevice device = VK_NULL_HANDLE; /**< @brief Handle to the logical device. */
private:
    /// @brief Pool desde el cual se asignan los command buffers.
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; /**< @brief Handle to the selected physical device. */

    /// @brief Cola de comandos de graphics, donde se envían los command buffers de dibujo.
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    /// @brief Cola de presentación, usada para mostrar imágenes en la superficie.
    VkQueue presentQueue = VK_NULL_HANDLE;

    /// @brief Índice de la queue family que soporta operaciones de graphics.
    uint32_t graphicsQueueFamilyIndex = 0xFFFFFFFF;

    /// @brief Índice de la queue family que soporta presentación en la superficie.
    uint32_t presentQueueFamilyIndex = 0xFFFFFFFF;
   
};
#endif // VULKAN_DEVICE_H