#include "Pipeline.h"
#include <memory>
#include <iostream>




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
 
Pipeline::Pipeline(VulkanDevice* vulkanDevice, VkRenderPass renderPass, PipelineConfig& config)
{
    this->vulkanDevice = vulkanDevice;
    VkDevice device = vulkanDevice->device;
    
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    if (!loadShader(config.shaderName, vertShaderModule, "vert") || !loadShader(config.shaderName, fragShaderModule, "frag")) {
        error = true;
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

    // 2. Vertex input (vacío por ahora, si el shader genera vértices internamente)
        // 2. ---- Aquí usas tu VertexLayout dinámico en vez del struct fijo ----

    VertexLayout layout(config.vertexAttributes);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &layout.binding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(layout.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = layout.attributes.data();

    // 3. Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 4. Viewport y scissor (dinámicos, recomendado)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    // 5. Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = config.polygonMode;
    rasterizer.lineWidth               = config.lineWidth;
    rasterizer.cullMode                = config.cullMode;
    rasterizer.frontFace               = config.frontFace;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // 6. Multisampling (deshabilitado)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 7. Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // 8. Pipeline layout (vacío por ahora: sin descriptor sets ni push constants)
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT; // o VERTEX_BIT según dónde esté el uniform
    uboBinding.pImmutableSamplers = nullptr;

VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &uboBinding;
vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cout << ("(VULKAN) Error in createPipeline(): No se pudo crear el pipeline layout.") << std::endl;
        error = true;
        return;
    }

    // 9. Crear el pipeline gráfico
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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cout << ("(VULKAN) Error in createPipeline(): No se pudo crear el graphics pipeline.") << std::endl;
        error = true;
        return;
    }

    // Los shader modules ya no se necesitan una vez creado el pipeline
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);


    #ifdef _DEBUG
    std::cout << "(VULKAN) Pipeline gráfico creado correctamente." << std::endl;
    #endif

    
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
