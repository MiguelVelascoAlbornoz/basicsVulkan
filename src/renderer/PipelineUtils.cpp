//
// Created by migue on 09/08/2026.
//

#include "PipelineUtils.h"


#include <memory>
#include <iostream>
#include "VulkanDevice.h"
/**
 * @brief Reads a binary file and stores its contents in a buffer.
 * @param filename The path to the binary file to read.
 * @param buffer
 */
bool PipelineUtils::readFile(const std::string& filename, std::vector<char> &buffer) {
    FILE* file = nullptr;
    file = fopen(filename.c_str(), "rb");
    if (!file) {
        return false;
    }
    char tempByte;
    //Hasta llegar a eof continuar leyendo un byte a la vez y guardarlo en el buffer
    while (fread(&tempByte, sizeof(char), 1, file) == 1) {
        buffer.push_back(tempByte);
    }
    fclose(file);
    return true;

}

std::string PipelineUtils::execCommand(const std::string& cmd) {
    std::vector<char> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif

    if (!pipe) throw std::runtime_error("popen() falló");
    char tempByte;
    while (fread(&tempByte, sizeof(char), 1, pipe.get()) == 1) {
        buffer.push_back(tempByte);
    }
    return std::string(buffer.begin(), buffer.end());
}

VkShaderModule PipelineUtils::createShaderModule(const std::vector<char> &code, const VkDevice device)
{

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << ("createShaderModule(): No se pudo crear el shader module.") << std::endl;
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool PipelineUtils::loadShader(std::string shaderName, VkShaderModule &shaderModule, const std::string& shaderType,VkDevice device, std::vector<std::string>& macros) {

    const std::string shaderPath = std::string("assets/shaders/"+shaderType+"/") + shaderName + "."+shaderType;

    //Compilar el codigo de cada uno
    const std::string shaderPathSPV = std::string("assets/shaders/compilated/") + shaderName + "."+shaderType+".spv";

#ifdef _DEBUG
    std::cout << "(VULKAN) Compilando shader: " << shaderPath << std::endl;
#endif
    std::string cmd = "glslc ";
    if (macros.size() % 2 != 0 || macros.size() == 1) {
        std::cout << "Formato del string de macros no respetado." << std::endl;
    }
    for (size_t i = 0; i < macros.size(); i+=2) {
        cmd = cmd +" -D"+macros[i]+"="+macros[i+1];
    }
    cmd = cmd + " " + shaderPath + " -o " + shaderPathSPV;
    #ifdef _DEBUG
    std::cout << "(SHADER) Comando: " << cmd << std::endl;
    #endif
    const std::string commandOut = PipelineUtils::execCommand(cmd);

    std::cout << commandOut << std::endl;

    // if (result) {
    //     std::cout << ("(VULKAN) Error in loadShaders(): Error al ejecutar el compilador del shader: "+std::string(shaderName))+"."+shaderType << std::endl;
    //     return false;
    // }

    std::vector<char> shaderCode;
    if (const bool result = PipelineUtils::readFile(shaderPathSPV, shaderCode); !result) {
        std::cout << ("(VULKAN) Error in loadShaders(): Shader \""+ std::string(shaderName) +"\" no compilado.") << std::endl;
        return false;
    }
    shaderModule = PipelineUtils::createShaderModule(shaderCode,device);

    return true;
}
bool PipelineUtils::createImage(VulkanDevice* device,
    int width, int height, VkFormat format, VkImageUsageFlags usage, int samples,VkImageAspectFlags aspectMask,VkImage& image, VkDeviceMemory& memory, VkImageView& imageView
)
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
    // COLOR_ATTACHMENT para renderizar, TRANSFER_SRC para poder copiarla despues a un buffer (debug PNG)
    imageInfo.usage         = usage;//Color usageVK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = static_cast<VkSampleCountFlagBits>(1 << samples);;
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

    if (vkCreateImageView(dev, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el image view de color." << std::endl;
        return false;
    }
    return true;
}
void PipelineUtils::createSampler(VkDevice device,
    VkSampler& sampler ,
    VkFilter magFilter ,VkFilter minFilter,
    VkBorderColor borderColor
)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;//VK_FILTER_LINEAR; // o NEAREST si prefieres depth sin filtrar
    samplerInfo.minFilter = minFilter;//VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = borderColor;//VK_BORDER_COLOR_INT_OPAQUE_WHITE; // 1.0 = "infinitamente lejos" fuera de rango
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
}

VkFormat PipelineUtils::dxgiToVulkanFormat(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat) {
    case DXGI_FORMAT_B8G8R8A8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return VK_FORMAT_R8G8B8A8_SRGB;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:    return VK_FORMAT_R16G16B16A16_SFLOAT;
    case DXGI_FORMAT_R10G10B10A2_UNORM:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    default:
        std::cerr << "(PIPELINE) Formato DXGI no soportado: " << static_cast<int>(dxgiFormat) << std::endl;
        return VK_FORMAT_UNDEFINED;
    }
}