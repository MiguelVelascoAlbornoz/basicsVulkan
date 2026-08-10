#include "Image.h"
#include "VulkanDevice.h"
#include "stbImage/stb_image.h"
#include <iostream>
#include <algorithm>
#include "stbImage/stb_image_write.h"

struct FormatInfo {
    int bytesPerTexel;
    int channels;
    bool isFloat;
};

static FormatInfo getFormatInfo(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UNORM:            return {1, 1, false};
    case VK_FORMAT_R8G8_UNORM:          return {2, 2, false};
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:       return {4, 4, false};
    case VK_FORMAT_R32_SFLOAT:          return {4, 1, true};
    case VK_FORMAT_R32G32_SFLOAT:       return {8, 2, true};
    case VK_FORMAT_R32G32B32A32_SFLOAT: return {16, 4, true};
    default:
        std::cerr << "(IMAGE) Formato no soportado en saveColorImageToPNG" << std::endl;
        return {0, 0, false};
    }
}

Image::Image(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat format,
             VkImageUsageFlags usage, VkImageAspectFlags aspectMask, int samplesPower)
    : device(device),width(width), height(height), format(format), usage(usage), aspectMask(aspectMask),
       samples(samplesPower)
{
    create();

}

Image::~Image()
{
    if (!device) return; // por si loadFromFile fallo antes de asignar device (ver nota abajo)
    VkDevice dev = device->device;

    if (view != VK_NULL_HANDLE)   vkDestroyImageView(dev, view, nullptr);
    if (image != VK_NULL_HANDLE)  vkDestroyImage(dev, image, nullptr);
    if (memory != VK_NULL_HANDLE) vkFreeMemory(dev, memory, nullptr);
    if (sampler != VK_NULL_HANDLE) vkDestroySampler(dev, sampler, nullptr);
}

void Image::createSampler( VkFilter magFilter, VkFilter minFilter,
    VkBorderColor borderColor)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;//VK_FILTER_LINEAR; // o NEAREST si prefieres depth sin filtrar
    samplerInfo.minFilter = minFilter;//VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = borderColor;//VK_BORDER_COLOR_INT_OPAQUE_WHITE; // 1.0 = "infinitamente lejos" fuera de rango
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    vkCreateSampler(this->device->device, &samplerInfo, nullptr, &sampler);
}

bool Image::create()
{
    VkDevice dev = device->device;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage         = usage;//Color usageVK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(dev, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear la imagen de color." << std::endl;
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(dev, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo alocar memoria para la imagen de color." << std::endl;
        return false;
    }
    vkBindImageMemory(dev, image, memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;
    viewInfo.components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                             .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask     = aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(dev, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el image view de color." << std::endl;
        return false;
    }
    createSampler(VK_FILTER_NEAREST,VK_FILTER_NEAREST,VK_BORDER_COLOR_INT_OPAQUE_WHITE);
    return true;
}

void Image::transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout,
                             VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = currentLayout;      // <-- ya no lo tienes que pasar tú
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    // srcAccessMask/dstAccessMask según el par de layouts — puedes tabular los casos
    // comunes (UNDEFINED->TRANSFER_DST, TRANSFER_DST->SHADER_READ_ONLY, ->GENERAL, etc.)

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    currentLayout = newLayout; // se actualiza el estado
}
void Image::saveColorImageToPNG(const std::string& filename) const
{
    VkDevice dev = device->device;

    FormatInfo info = getFormatInfo(format);
    if (info.bytesPerTexel == 0) return; // formato no soportado, ya logueado

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * info.bytesPerTexel;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
    void* data;
    vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &data);
    std::vector<unsigned char> rawPixels(imageSize);
    memcpy(rawPixels.data(), data, imageSize);
    vkUnmapMemory(dev, stagingMemory);

    if (info.isFloat) {
        // stb_write_png no soporta float directo: hay que convertir a 8-bit,
        // normalmente clampeando/remapeando el rango que uses (ej. [0,1] o [-1,1])
        std::vector<unsigned char> pixels8(width * height * info.channels);
        const float* floatData = reinterpret_cast<const float*>(rawPixels.data());
        for (size_t i = 0; i < pixels8.size(); i++) {
            float v = std::clamp(floatData[i], 0.0f, 1.0f); // ajusta el rango según tu caso
            pixels8[i] = static_cast<unsigned char>(v * 255.0f);
        }
        stbi_write_png(filename.c_str(), width, height, info.channels,
                        pixels8.data(), width * info.channels);
    } else {
        if (format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB) {
            for (size_t i = 0; i < rawPixels.size(); i += 4) {
                std::swap(rawPixels[i], rawPixels[i + 2]);
            }
        }
        stbi_write_png(filename.c_str(), width, height, info.channels,
                        rawPixels.data(), width * info.channels);
    }

    vkDestroyBuffer(dev, stagingBuffer, nullptr);
    vkFreeMemory(dev, stagingMemory, nullptr);
    std::cout << "Imagen guardada en: " << filename << std::endl;
}
Image* Image::loadFromFile(VulkanDevice* device, const std::string& path, VkImageUsageFlags usage,VkImageLayout newLayout) {
    int w, h, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (!pixels) { std::cerr << "(IMAGE) No se pudo cargar: " << path << std::endl; return nullptr; }
    channels = channels == 3? 4 : channels;
    VkDeviceSize size = w * h * channels;
    VkBuffer staging; VkDeviceMemory stagingMem;
    device->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);

    void* mapped;
    vkMapMemory(device->device, stagingMem, 0, size, 0, &mapped);
    memcpy(mapped, pixels, size);
    vkUnmapMemory(device->device, stagingMem);
    stbi_image_free(pixels);

    VkFormat format;
    switch (channels)
    {
    case 1:
        format = VK_FORMAT_R8_UNORM;
        break;
    case 2:
        format = VK_FORMAT_R8G8_UNORM;
        break;
    case 3:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        break;
    case 4:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        break;
    default:
        std::cout << "Error finding image format: " << path << std::endl;
        return nullptr;
    }
    auto* img = new Image(device, w, h, format,usage,VK_IMAGE_ASPECT_COLOR_BIT,0);

    VkCommandBuffer cmd = device->beginSingleTimeCommands();
    img->transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    device->endSingleTimeCommands(cmd);

    device->copyBufferToImage(staging, img->getImage(), w, h);

    cmd = device->beginSingleTimeCommands();
    img->transitionLayout(cmd, newLayout,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    device->endSingleTimeCommands(cmd);

    vkDestroyBuffer(device->device, staging, nullptr);
    vkFreeMemory(device->device, stagingMem, nullptr);
    return img;
}

