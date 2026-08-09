//
// Created by migue on 09/08/2026.
//

#ifndef BASICSVULKAN_PIPELINEUTILS_H
#define BASICSVULKAN_PIPELINEUTILS_H
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
class UniformBuffer;
struct ImageBinding
{
    VkImageView image = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageLayout layout;
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
    static VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice device);
    static bool loadShader(std::string shaderName, VkShaderModule& shaderModule, const std::string& shaderType,VkDevice device);
    static bool readFile(const std::string& filename, std::vector<char>& buffer);
    static std::string execCommand(const std::string& cmd);
};


#endif //BASICSVULKAN_PIPELINEUTILS_H
