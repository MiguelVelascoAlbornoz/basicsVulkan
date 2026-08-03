#ifndef BASICSVULKAN_FRAMEBUFFEROBJECT_H
#define BASICSVULKAN_FRAMEBUFFEROBJECT_H

#include "vulkan/vulkan_core.h"
#include <string>

class VulkanDevice;

class FrameBufferObject
{
public:


    [[nodiscard]] VkSampler getColorSampler() const { return colorSampler; }
    FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height,
                       VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
                       bool useDepth = true); // <-- nuevo parametro, default activado
    ~FrameBufferObject();

    void beginRenderPass(VkCommandBuffer cmd);
    void endRenderPass(VkCommandBuffer cmd);

    void saveColorImageToPNG(const std::string& filename);

    [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
    [[nodiscard]] VkImage getColorImage() const { return colorImage; }
    [[nodiscard]] VkImageView getColorImageView() const { return colorImageView; }
    [[nodiscard]] VkFramebuffer getFramebuffer() const { return framebuffer; }
    [[nodiscard]] VkExtent2D getExtent() const { return { width, height }; }
    [[nodiscard]] bool hasDepth() const { return useDepth; }

    bool error = false;

private:
    void createColorResources();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffer();
    VkFormat findDepthFormat() const;
    VkSampler colorSampler = VK_NULL_HANDLE;
    VulkanDevice* device = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat colorFormat;
    bool useDepth = true;

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};

#endif //BASICSVULKAN_FRAMEBUFFEROBJECT_H