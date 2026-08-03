#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H




class Window;
class VulkanDevice;
#include <Vulkan/vulkan.h>
#include <vector>
/**
 * @brief The SwapChain class encapsulates the creation and management of a Vulkan swapchain, which is a series of images that are presented to the screen in a loop.
 * @details El swapChain necesita su Device, es un vector de VKImages, VKImagesViews, VkFramebuffers.
 * Para renderizar al final se usa el frameBuffer actual
 */
class SwapChain {
    
public:
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
    SwapChain(VulkanDevice *vulkanDevice, Window *window, VkRenderPass renderPass, VkSurfaceFormatKHR chosenFormat);

    ~SwapChain();
    /**
     * @brief Creates the image views for each image in the swapchain, so they can be used as attachments in the framebuffers.
     * @details A VkImage by itself cannot be accessed directly by the pipeline; it needs a VkImageView that describes how to interpret it (format, type, mip levels, array layers). Creates one VkImageView per VkImage in swapchainImages, storing them in swapchainImageViews.
     */
    bool createImageViews();
    bool recreateSwapChain(VkRenderPass renderPass, Window *window);
    VkFormat getSwapchainImageFormat() const { return swapchainFormat.format; } /**< @brief Get the format of the swapchain images. */
    bool error = false; /**< @brief Flag to indicate if there was an error during initialization. */
    VkFramebuffer getFramebuffer(size_t index) const { return swapchainFramebuffers[index]; } /**< @brief Get the framebuffer for a specific swapchain image index. */
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; } /**< @brief Get the extent (width and height) of the swapchain images. */
    /**
     * @brief Acquires the next available image from the swapchain for rendering.
     * @returns true if the image was successfully acquired, false if the swapchain is out of date or suboptimal (requiring recreation).
     */
    bool presentImage(uint32_t imageIndex, VkSemaphore *renderFinishedSemaphore);
    /**
     * @brief Presents the rendered image to the screen using the specified present queue.
     * @returns true if the image was successfully presented, false if the swapchain is out of date or suboptimal (requiring recreation).
     */
    bool acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t* pImageIndex);

    [[nodiscard]] int getImageCount() const { return static_cast<int>(swapchainImages.size()); } /**< @brief Get the number of images in the swapchain. */


private:
    /**
     * @brief Creates one VkFramebuffer per swapchain image view, binding them to renderPass.
     * @details A framebuffer is the concrete binding between a render pass and the actual memory (image views) it will render into. Requires createImageViews() and createRenderPass() to have run first. Stored in swapchainFramebuffers, indexed the same way as swapchainImageViews.
     */
    bool createFramebuffers(VkRenderPass renderPass);
    void destroySwapChain();
    bool createSwapChain(Window* window,VkRenderPass renderPass);

    void destroyFrameBuffers();

    VkSurfaceKHR surface = VK_NULL_HANDLE; /**< @brief Handle to the Vulkan surface associated with the window. */
    VulkanDevice* vulkanDevice = nullptr;
    VkPresentModeKHR presentMode; /**< @brief Chosen present mode (FIFO, MAILBOX, etc.) for the swapchain. */
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;

    /// @brief Vistas (VkImageView) de cada imagen de la swapchain, necesarias para usarlas como attachments.
    std::vector<VkImageView> swapchainImageViews;

    /// @brief Formato de color elegido para la swapchain (ej. VK_FORMAT_B8G8R8A8_SRGB).
    VkSurfaceFormatKHR swapchainFormat;

    /// @brief Resolución (ancho x alto) de las imágenes de la swapchain.
    VkExtent2D swapchainExtent;

    /// @brief Framebuffers, uno por cada image view de la swapchain, usados en vkCmdBeginRenderPass.
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE; /**< @brief Handle to the render pass used for rendering into the swapchain framebuffers. */

};
#endif // SWAPCHAIN_H