#ifndef BASICSVULKAN_FRAMEBUFFEROBJECT_H
#define BASICSVULKAN_FRAMEBUFFEROBJECT_H

#include "vulkan/vulkan_core.h"
#include <string>
#include <vector>

class VulkanDevice;
class Pipeline;

class FrameBufferObject
{
public:

    [[nodiscard]] VkSampler getColorSampler() const { return colorSampler; }
    FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height,
                       VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
                       bool useDepth = true, bool depthSamplerEnabled = false, int multiSamplerPower = 0);
    void createResources();
    void destroyResources();
    ~FrameBufferObject();

    void beginRenderPass(VkCommandBuffer cmd);
    void endRenderPass(VkCommandBuffer cmd);

    void saveColorImageToPNG(const std::string& filename);

    [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
    // Con MSAA activo, esta view apunta al resolve attachment (samples=1), sampleable normalmente.
    [[nodiscard]] VkImageView getColorImageView() const
    {
    return    colorImageView;
    }
    int getMultiSamplerPower() const { return multiSamplerPower; }
    [[nodiscard]] VkFramebuffer getFramebuffer() const { return framebuffer; }
    [[nodiscard]] VkExtent2D getExtent() const { return { width, height }; }
    [[nodiscard]] bool hasDepth() const { return useDepth; }
    using SceneFunction = void(*)(VkCommandBuffer);
    void addScene(SceneFunction scene);
    void removeScene(SceneFunction scene);
    bool error = false;
    void renderScenes(VkCommandBuffer cmd);
    void recreateResources()  { destroyResources(); createResources(); };

    [[nodiscard]] VkSampler getDepthSampler() const { return depthSampler; }
    [[nodiscard]] VkImageView getDepthImageView() const { return depthImageView; }
    void changeResolution(int newWidth, int newHeight);

    /** @brief Cambia el sample count del FBO y recrea en cascada: color/depth/resolve,
     *  render pass, framebuffer, y todos los pipelines dependientes registrados. */
    void changeMultiSamplerPower(int newMultiSamplerPower);
    int getSamplesPower() const { return multiSamplerPower; }

    /** @brief Registra un pipeline que dibuja usando este FBO como render target,
     *  para que se recree automáticamente cuando cambia el sample count. */
    void registerDependentPipeline(Pipeline* pipeline) { dependentPipelines.push_back(pipeline); }

private:

    std::vector<SceneFunction> scenes;
    std::vector<Pipeline*> dependentPipelines;

    void createColorResources();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffer();
    VkFormat findDepthFormat() const;
    VkSampler colorSampler = VK_NULL_HANDLE;
    VkSampler depthSampler = VK_NULL_HANDLE;
    VulkanDevice* device = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat colorFormat;
    bool depthSamplerEnabled;
    bool useDepth;
    int multiSamplerPower;
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