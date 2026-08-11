#ifndef IMAGE_H
#define IMAGE_H


#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
class VulkanDevice;



class Image {
public:

    /** @brief Crea una imagen vacía en VRAM (render target, storage image, etc). */
    Image(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat format,
          VkImageUsageFlags usage, VkImageAspectFlags aspectMask, int samplesPower = 0, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);

    ~Image(); // libera VRAM automáticamente (RAII, como ya hace Mesh/UniformBuffer)
    void createSampler(VkFilter magFilter , VkFilter minFilter, VkBorderColor borderColor);
    bool create();
    [[nodiscard]] VkImage getImage() const { return image; }
    [[nodiscard]] VkSampler getSampler() const { return sampler; }
    // No copiable (mismo motivo que ComputePipeline)
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    /** @brief Carga una imagen desde disco (PNG/JPG vía stb_image) a VRAM. */
    static Image* loadFromFile(VulkanDevice* device, const std::string& path, VkImageUsageFlags usage, VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    void saveColorImageToPNG(const std::string& filename) const;

    /** @brief Transiciona el layout y actualiza el estado interno, para que
     *  el caller no tenga que recordar el layout anterior. */
    void transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout,
                           VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

    [[nodiscard]] VkImageView getView() const { return view; }
    [[nodiscard]] VkImageLayout getCurrentLayout() const { return currentLayout; }
    [[nodiscard]] VkExtent2D getExtent() const { return {width, height}; }
    [[nodiscard]] VkFormat getFormat() const { return format; }

    bool error = false;
    static std::unordered_map<std::string, Image*> images;

private:

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;

    VulkanDevice* device;
    uint32_t width;
    uint32_t height;
    VkFormat format;

    VkImageUsageFlags usage;
    VkImageAspectFlags aspectMask;
    VkSampler sampler;
    int samples;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED; // tracking automático


};

#endif