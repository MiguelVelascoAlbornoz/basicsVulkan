#include "ComputePipeline.h"
#include "VulkanDevice.h"

#include <iostream>
#include "UniformBuffer.h"


ComputePipeline::ComputePipeline(VulkanDevice* device, VkDescriptorPool descriptorPool, ComputePipelineConfig& config) :
    vulkanDevice(device), descriptorPool(descriptorPool)
{
    this->config = config;
    buildPipeline();
}

ComputePipeline::~ComputePipeline()
{
    VkDevice device = vulkanDevice->device;

    if (device != VK_NULL_HANDLE) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
}

void ComputePipeline::buildPipeline()
{
    // Por si buildPipeline() se llama de nuevo (p.ej. desde recreate()) tras
    // un fallo previo, empezamos limpios en vez de arrastrar el error viejo.
    error = false;

    //CREATIKNG DESCRIPTOR SET LAYOUT
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(config.uniformObjects.size() + config.images.size());
    uint32_t bindingCount = 0;
    for (size_t i = 0; i < config.uniformObjects.size(); i++, bindingCount++) {
        layoutBindings[bindingCount].binding = bindingCount;
        layoutBindings[bindingCount].descriptorType = config.uniformObjects[i].type;
        layoutBindings[bindingCount].descriptorCount = 1;
        layoutBindings[bindingCount].stageFlags = config.uniformObjects[i].stageFlags;
        layoutBindings[bindingCount].pImmutableSamplers = nullptr;
    }
    for (size_t i = 0; i < config.images.size(); i++, bindingCount++) {
        layoutBindings[bindingCount].binding = bindingCount;
        layoutBindings[bindingCount].descriptorType = config.images[i].type;
        layoutBindings[bindingCount].descriptorCount = 1;
        layoutBindings[bindingCount].stageFlags = config.images[i].stageFlags;
        layoutBindings[bindingCount].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();
    if (vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in buildPipeline(): No se pudo crear el descriptor set layout." << std::endl;
        error = true;
        return;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(vulkanDevice->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in buildPipeline(): No se pudo alocar el descriptor set." << std::endl;
        error = true;
        return;
    }
    //END DESCRIPTOR SET LAYOUT
    //CREATING LAYOUT
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkPushConstantRange pushConstantRange{};

    if (config.pushConstantsSize != 0) {
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = config.pushConstantsSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(vulkanDevice->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in buildPipeline(): No se pudo crear el pipeline layout." << std::endl;
        error = true;
        return;
    }
    //LAYOUT END
    //STAGING SHADER
    VkShaderModule shaderModule;

    if (!PipelineUtils::loadShader(config.shaderName, shaderModule, "comp", vulkanDevice->device)) {
        error = true;
        return;
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName  = "main";
    //STAIGN SHADER END

    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.pNext  = nullptr;
    createInfo.stage  = stageInfo;
    createInfo.layout = pipelineLayout;

    if (vkCreateComputePipelines(
        vulkanDevice->device,
        VK_NULL_HANDLE,
        1,
        &createInfo,
        nullptr,
        &pipeline) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in buildPipeline(): No se pudo crear el compute pipeline." << std::endl;
        error = true;
        vkDestroyShaderModule(vulkanDevice->device, shaderModule, nullptr);
        return;
    }

    vkDestroyShaderModule(vulkanDevice->device, shaderModule, nullptr);

    #ifdef _DEBUG
    std::cout << "(VULKAN) Compute pipeline creado correctamente." << std::endl;
    #endif
    updateDescriptorSet();
}

void ComputePipeline::updateDescriptorSet(std::vector<UniformBinding> uniformObjects, std::vector<ImageBinding> images)
{
    config.images = std::move(images);
    config.uniformObjects = std::move(uniformObjects);
    updateDescriptorSet();
}

void ComputePipeline::updateDescriptorSet()
{
    std::vector<UniformBinding> uniformObjects = config.uniformObjects;
    std::vector<ImageBinding> images = config.images;
    VkDevice device = vulkanDevice->device;

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos(uniformObjects.size());
    std::vector<VkDescriptorImageInfo> imageInfos(images.size());

    uint32_t bindingCount = 0;
    for (size_t i = 0; i < uniformObjects.size(); i++, ++bindingCount) {
        if (uniformObjects[i].uniformBuffer == nullptr) continue;
        uniformObjects[i].uniformBuffer->getBufferInfo(bufferInfos[i]);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = bindingCount;
        write.dstArrayElement = 0;
        write.descriptorType = uniformObjects[i].type;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfos[i];
        writes.push_back(write);
    }

    for (size_t i = 0; i < images.size(); i++, ++bindingCount) {
        if (images[i].image == VK_NULL_HANDLE) continue;
        imageInfos[i].imageLayout = images[i].layout;
        imageInfos[i].imageView   = images[i].image;
        imageInfos[i].sampler     = images[i].sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = bindingCount;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = images[i].type;
        write.pImageInfo = &imageInfos[i];
        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    config.images = std::move(images);
    config.uniformObjects = std::move(uniformObjects);
}

void ComputePipeline::destroyPipelineObjects()
{
    const VkDevice device = vulkanDevice->device;

    // 1. Liberar el descriptor set ANTES de destruir su layout
    if (descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
        descriptorSet = VK_NULL_HANDLE;
    }
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void ComputePipeline::recreate(const std::optional<ComputePipelineConfig>& newConfig)
{
    vkDeviceWaitIdle(vulkanDevice->device);
    if (newConfig) config = *newConfig;

    destroyPipelineObjects();
    buildPipeline();
}

void ComputePipeline::bind(VkCommandBuffer cmd) const
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

void ComputePipeline::pushConstants(VkCommandBuffer cmd, const void* data, uint32_t size, uint32_t offset) const
{
    if (config.pushConstantsSize == 0) {
        std::cout << "(VULKAN) Warning in pushConstants(): este pipeline no tiene push constants configuradas." << std::endl;
        return;
    }
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
}

void ComputePipeline::dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}