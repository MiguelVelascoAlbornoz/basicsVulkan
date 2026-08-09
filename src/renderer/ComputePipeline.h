#include <string>
#include <optional>
#include "Pipeline.h"
#include "vulkan/vulkan_core.h"
#include <vector>

#ifndef BASICSVULKAN_COMPUTEPIPELINE_H
#define BASICSVULKAN_COMPUTEPIPELINE_H

class VulkanDevice;

struct ComputePipelineConfig {

    std::vector<ImageBinding> images;
    std::vector<UniformBinding> uniformObjects;

    std::string shaderName = "compute";
    int pushConstantsSize = 0;
};

class ComputePipeline{

public:
    ComputePipeline(VulkanDevice* device, VkDescriptorPool descriptorPool, ComputePipelineConfig& config);
    ~ComputePipeline();

    // La clase posee handles de Vulkan (pipeline, layout, descriptor set...).
    // Copiarla causaría doble-liberación de esos handles, así que se prohíbe.
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    void buildPipeline();
    void updateDescriptorSet(std::vector<UniformBinding> uniformObjects, std::vector<ImageBinding> images);
    void updateDescriptorSet();
    void recreate(const std::optional<ComputePipelineConfig>& newConfig = std::nullopt);
    void destroyPipelineObjects();

    // Uso del pipeline dentro de un command buffer ya en grabación.
    void bind(VkCommandBuffer cmd) const;
    void pushConstants(VkCommandBuffer cmd, const void* data, uint32_t size, uint32_t offset = 0) const;
    static void dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);

    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    bool error = false;
private:
    ComputePipelineConfig config;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VulkanDevice* vulkanDevice;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};


#endif //BASICSVULKAN_COMPUTEPIPELINE_H