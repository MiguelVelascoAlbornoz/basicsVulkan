#include "Pipeline.h"
#include <memory>
#include <iostream>
#include <optional>
#include <cmath>

#include "VulkanDevice.h"
#include "VertexLayout.h"
#include "UniformBuffer.h"

VkShaderModule Pipeline::createShaderModule(const std::vector<char> &code)
{
    VkDevice device = vulkanDevice->device;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << ("createShaderModule(): No se pudo crear el shader module.") << std::endl;
        return nullptr;
    }
    return shaderModule;
}
/**
 * @brief Reads a binary file and stores its contents in a buffer.
 * @param filename The path to the binary file to read.
 * @param buffer
 */
static bool readFile(const std::string& filename, std::vector<char> &buffer) {
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

static std::string execCommand(const std::string& cmd) {
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
bool Pipeline::loadShader(std::string shaderName, VkShaderModule &shaderModule, std::string shaderType) {

    std::string shaderPath = std::string("assets/shaders/"+shaderType+"/") + shaderName + "."+shaderType;

    //Compilar el codigo de cada uno
    std::string shaderPathSPV = std::string("assets/shaders/compilated/") + shaderName + "."+shaderType+".spv";

    #ifdef _DEBUG
    std::cout << "(VULKAN) Compilando shader: " << shaderPath << std::endl;
    #endif
    std::string commandOut = execCommand(("glslc "+shaderPath+" -o "+shaderPathSPV).c_str());
    std::cout << commandOut << std::endl;

   // if (result) {
   //     std::cout << ("(VULKAN) Error in loadShaders(): Error al ejecutar el compilador del shader: "+std::string(shaderName))+"."+shaderType << std::endl;
   //     return false;
   // }

    std::vector<char> shaderCode;
    if (const bool result = readFile(shaderPathSPV, shaderCode); !result) {
        std::cout << ("(VULKAN) Error in loadShaders(): Shader \""+ std::string(shaderName) +"\" no compilado.") << std::endl;
        return false;
    }
    shaderModule = createShaderModule(shaderCode);

    return true;
}


void Pipeline::updateMSAASamples(int newValue)
{
    config.multisamplerSamples = newValue;
    recreate(this->config,this->renderPass);
}

Pipeline::Pipeline(VulkanDevice* vulkanDevice, VkRenderPass renderPass, PipelineConfig& config, VkDescriptorPool descriptorPool)
{
    this->vulkanDevice = vulkanDevice;
    this->renderPass = renderPass;
    this->descriptorPool = descriptorPool;
    this->config = config;
    buildPipeline();
}
void Pipeline::recreate(std::optional<PipelineConfig> newConfig, VkRenderPass newRenderPass)
{
    vkDeviceWaitIdle(vulkanDevice->device);
    if (newConfig)   config = *newConfig;
    if (newRenderPass != VK_NULL_HANDLE) renderPass = newRenderPass;

    destroyPipelineObjects();
    buildPipeline();
}
void Pipeline::destroyPipelineObjects()
{
    VkDevice device = vulkanDevice->device;

    // 1. Liberar el descriptor set ANTES de destruir su layout
    if (descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
        descriptorSet = VK_NULL_HANDLE;
    }
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
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

void Pipeline::buildPipeline()
{
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    if (!loadShader(config.shaderName, vertShaderModule, "vert")) {
        error = true;
        return;
    }
    if (!loadShader(config.shaderName, fragShaderModule, "frag")) {
        error = true;
        vkDestroyShaderModule(vulkanDevice->device, vertShaderModule, nullptr);
        return;
    }

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VertexLayout layout(config.vertexAttributes);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &layout.binding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(layout.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = layout.attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    std::vector dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = config.polygonMode;
    rasterizer.lineWidth               = config.lineWidth;
    rasterizer.cullMode                = config.cullMode;
    rasterizer.frontFace               = config.frontFace;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // ---- multisampling: ahora usa config.multisamplerSamples correctamente ----
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = config.sampleShadding ? VK_TRUE : VK_FALSE;
    multisampling.minSampleShading     = config.sampleShadding ? 1.0f : 0.0f;
    multisampling.rasterizationSamples = static_cast<VkSampleCountFlagBits>(1 << config.multisamplerSamples);

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // ---- descriptor set layout + set: SIEMPRE se recrean (ya no hay shouldCreateDescriptorSet) ----
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
    vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutInfo, nullptr, &descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(vulkanDevice->device, &allocInfo, &descriptorSet);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    VkPushConstantRange pushConstantRange{};

    if (config.pushConstantsSize != 0) {
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
        vkDestroyShaderModule(vulkanDevice->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->device, vertShaderModule, nullptr);
        return;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = config.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.pDepthStencilState  = config.depthTestEnable ? &depthStencil : nullptr;

    if (vkCreateGraphicsPipelines(vulkanDevice->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in buildPipeline(): No se pudo crear el graphics pipeline." << std::endl;
        error = true;
        vkDestroyShaderModule(vulkanDevice->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->device, vertShaderModule, nullptr);
        return;
    }

    vkDestroyShaderModule(vulkanDevice->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanDevice->device, vertShaderModule, nullptr);

    #ifdef _DEBUG
    std::cout << "(VULKAN) Pipeline gráfico creado correctamente." << std::endl;
    #endif

    updateDescriptorSet(config.uniformObjects, config.images);
}
void Pipeline::updateDescriptorSet( std::vector<UniformBinding> uniformObjects,  std::vector<ImageBinding> images)
{

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
        if (images[i].image == VK_NULL_HANDLE || images[i].sampler == VK_NULL_HANDLE) continue;
        imageInfos[i].imageLayout = images[i].layout;
        imageInfos[i].imageView   = images[i].image;
        imageInfos[i].sampler     = images[i].sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = bindingCount;
        write.descriptorCount = 1;
        write.descriptorType = images[i].type;
        write.pImageInfo = &imageInfos[i];
        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    config.images = std::move(images);
    config.uniformObjects = std::move(uniformObjects);
}

void Pipeline::updateShaders()
{
    vkDeviceWaitIdle(vulkanDevice->device);
    recreate(this->config,this->renderPass);
}

Pipeline::~Pipeline()
{
    VkDevice device = vulkanDevice->device;

    if (device != VK_NULL_HANDLE) {
        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
 
}
