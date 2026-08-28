//
// Created by migue on 09/08/2026.
//

#ifndef BASICSVULKAN_PIPELINEUTILS_H
#define BASICSVULKAN_PIPELINEUTILS_H
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "VulkanDevice.h"
class UniformBuffer;
class VulkanDevice;

struct ImageBinding
{
    VkImageView image = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
};
struct UniformBinding
{
    UniformBuffer* uniformBuffer = nullptr;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
};

class PipelineUtils
{
public:
    static VkFormat dxgiToVulkanFormat(DXGI_FORMAT dxgiFormat);
    static VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice device);

    /**
     * @param macros Deve ser un vector de string pero tiene que seguir el siguiente orden string[i] = nombre de la macro, string[i+1] = Valor de la macro, siendo i par.
     */
    static bool loadShader(std::string shaderName, VkShaderModule &shaderModule, const std::string &shaderType, VkDevice device, std::vector<std::
                               string> &macros);
    static bool createImage(VulkanDevice* device, int width, int height, VkFormat format, VkImageUsageFlags usage, int samples,
                     VkImageAspectFlags aspectMask, VkImage& image, VkDeviceMemory& memory, VkImageView& imageView);
    static void createSampler(VkDevice device, VkSampler& sampler, VkFilter magFilter, VkFilter minFilter,
                              VkBorderColor borderColor);
    static bool readFile(const std::string& filename, std::vector<char>& buffer);
    static std::string execCommand(const std::string& cmd);
};


#endif //BASICSVULKAN_PIPELINEUTILS_H
