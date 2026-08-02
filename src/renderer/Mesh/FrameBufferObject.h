#ifndef BASICSVULKAN_FRAMEBUFFEROBJECT_H
#define BASICSVULKAN_FRAMEBUFFEROBJECT_H

#include "vulkan/vulkan_core.h"
#include <string>

class VulkanDevice;

/**
 * @brief Encapsula un framebuffer offscreen (FBO) con un solo color attachment.
 * Pensado para renderizar la escena antes de pasarla a un pipeline de post-proceso
 * que finalmente dibuja sobre el swapchain.
 */
class FrameBufferObject
{
public:
    FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height,
                       VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM);
    ~FrameBufferObject();

    void beginRenderPass(VkCommandBuffer cmd);
    void endRenderPass(VkCommandBuffer cmd);

    /** @brief Copia la imagen de color a un buffer host-visible y la guarda como PNG. Solo para debug. */
    void saveColorImageToPNG(const std::string& filename);

    [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
    [[nodiscard]] VkImage getColorImage() const { return colorImage; }
    [[nodiscard]] VkImageView getColorImageView() const { return colorImageView; }
    [[nodiscard]] VkFramebuffer getFramebuffer() const { return framebuffer; }
    [[nodiscard]] VkExtent2D getExtent() const { return { width, height }; }

    bool error = false;

private:
    void createColorResources();
    void createRenderPass();
    void createFramebuffer();

    VulkanDevice* device = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat colorFormat;

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};

#endif //BASICSVULKAN_FRAMEBUFFEROBJECT_H