#ifndef IMAGE_H
#define IMAGE_H


#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // HANDLE
class VulkanDevice;

struct SampleConfig {
    VkFilter magFilter = VK_FILTER_NEAREST;
    VkFilter minFilter = VK_FILTER_NEAREST;
    VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    VkSamplerAddressMode adressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

class Image {
public:
    struct KeyedMutexSync {
        bool active = false;
        UINT64 acquireKey = 0;
        UINT64 releaseKey = 0;
        uint32_t timeoutMs = 1000;
    };

    void setKeyedMutexSync(UINT64 acquireKey, UINT64 releaseKey, uint32_t timeoutMs = 1000) {
        keyedMutexSync = { true, acquireKey, releaseKey, timeoutMs };
    }
    [[nodiscard]] const KeyedMutexSync& getKeyedMutexSync() const { return keyedMutexSync; }
    [[nodiscard]] VkDeviceMemory getMemory() const { return memory; }
    /** @brief Destruye la imagen/view/memoria actuales y las recrea con la nueva resolución.
         *  El sampler NO se toca (no depende del tamaño). El layout vuelve a UNDEFINED,
         *  el caller es responsable de transicionarlo de nuevo antes de usarla. */
    void resize(uint32_t newWidth, uint32_t newHeight);
    /** @brief Crea una imagen vacía en VRAM (render target, storage image, etc). */
    Image(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat format,
          VkImageUsageFlags usage, VkImageAspectFlags aspectMask,SampleConfig sampleConfig, int samplesPower = 0, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);

    ~Image(); // libera VRAM automáticamente (RAII, como ya hace Mesh/UniformBuffer)
    void createSampler(VkFilter magFilter, VkFilter minFilter, VkBorderColor borderColor, VkSamplerAddressMode adressMode);
    bool create();
    void destroyImageResources();
    [[nodiscard]] VkImage getImage() const { return image; }
    [[nodiscard]] VkSampler getSampler() const { return sampler; }
    // No copiable (mismo motivo que ComputePipeline)
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    /** @brief Importa una textura compartida de D3D11 (handle NT obtenido con
         *  IDXGIResource1::CreateSharedHandle) como un VkImage con memoria externa.
         *  @details No copia datos: el VkImage resultante apunta a la MISMA memoria
         *  GPU que la textura D3D11. Cierra el handle una vez importado (es un NT
         *  handle propio, Vulkan solo lo referencia durante el import). El caller
         *  sigue siendo dueño de sincronizar D3D11 <-> Vulkan (keyed mutex o similar). */
    static Image* importFromD3D11Handle(VulkanDevice* device, HANDLE sharedHandle,
                                         uint32_t width, uint32_t height, VkFormat format,
                                         VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                                         SampleConfig config = {});
    /** @brief Carga una imagen desde disco (PNG/JPG vía stb_image) a VRAM. */
    static Image* loadFromFile(VulkanDevice* device, const std::string& path, VkImageUsageFlags usage, VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SampleConfig config = {});
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
    /** @brief Tag para el constructor "vacío" usado por los imports: no llama a create(). */
    KeyedMutexSync keyedMutexSync;
    struct ImportTag {};
    Image(VulkanDevice* device, ImportTag);
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;

    VulkanDevice* device;
    uint32_t width;
    uint32_t height;
    VkFormat format;

    VkImageUsageFlags usage;
    VkImageAspectFlags aspectMask;
    SampleConfig sampleConfig;
    VkSampler sampler = VK_NULL_HANDLE;
    int samples;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED; // tracking automático


};

#endif