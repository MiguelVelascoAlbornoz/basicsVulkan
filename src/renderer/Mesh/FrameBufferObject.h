#ifndef BASICSVULKAN_FRAMEBUFFEROBJECT_H
#define BASICSVULKAN_FRAMEBUFFEROBJECT_H

#include "vulkan/vulkan_core.h"
#include <string>
#include <vector>


class VulkanDevice;

class FrameBufferObject
{
public:


    [[nodiscard]] VkSampler getColorSampler() const { return colorSampler; }
    FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height,
                       VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
                       bool useDepth = true,bool createDepthSampler = false); // <-- nuevo parametro, default activado
    ~FrameBufferObject();

    void beginRenderPass(VkCommandBuffer cmd);
    void endRenderPass(VkCommandBuffer cmd);

    void saveColorImageToPNG(const std::string& filename);

    [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
    [[nodiscard]] VkImage getColorImage() const { return colorImage; }
    [[nodiscard]] VkImageView getColorImageView() const { return colorImageView; }
    [[nodiscard]] VkFramebuffer getFramebuffer() const { return framebuffer; }
    [[nodiscard]] VkExtent2D getExtent() const { return { width, height }; }
    [[nodiscard]] bool hasDepth() const { return useDepth; }
    using SceneFunction = void(*)(VkCommandBuffer);
    void addScene(SceneFunction scene);
    void removeScene(SceneFunction scene);
    bool error = false;
    void renderScenes(VkCommandBuffer cmd);

    [[nodiscard]] VkSampler getDepthSampler() const { return depthSampler; }
    [[nodiscard]] VkImageView getDepthImageView() const { return depthImageView; }
    void changeResolution(int newWidth, int newHeight);
private:

    std::vector<SceneFunction> scenes;
    void createColorResources();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffer();
    VkFormat findDepthFormat() const;
    VkSampler colorSampler = VK_NULL_HANDLE;
    VkSampler depthSampler = VK_NULL_HANDLE;
    VulkanDevice* device = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat colorFormat;
    bool createDepthSampler;
    bool useDepth;

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};

#endif //BASICSVULKAN_FRAMEBUFFEROBJECT_H
/*
void debugTestFBO(App* app)
{
    VulkanDevice* device = app->renderer->getVulkanDevice();

    // Descriptor pool propio para no pelear con el pool de la app
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = 1;

    VkDescriptorPool testDescriptorPool;
    vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &testDescriptorPool);

    FrameBufferObject fbo(device, 800, 600);
    if (fbo.error) { std::cerr << "Error creando FBO de test." << std::endl; return; }

    Pipeline::PipelineConfig config;
    config.vertexAttributes  = { AttribType::VEC3, AttribType::VEC3 };
    config.pushConstantsSize = sizeof(Model::ModelUBO);

    Pipeline testPipeline(device, fbo.getRenderPass(), config, testDescriptorPool);
    if (testPipeline.error) { std::cerr << "Error creando pipeline de test." << std::endl; return; }

    VkDescriptorBufferInfo bufferInfo{};
    VkWriteDescriptorSet write = Uniforms::cameraUniform->getWriteDescriptor(testPipeline.descriptorSet, 0, bufferInfo);
    vkUpdateDescriptorSets(device->device, 1, &write, 0, nullptr);

    VkCommandBuffer cmd = device->beginSingleTimeCommands();

    fbo.beginRenderPass(cmd);
    testPipeline.bind(cmd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, testPipeline.getPipelineLayout(),
                             0, 1, &testPipeline.descriptorSet, 0, nullptr);

    Model cubeModel;
    cubeModel.mesh = Meshes::cubeMesh;
    cubeModel.setTranslation(vec3(0.0f));
    cubeModel.setRotation(vec3(0.4f, 0.6f, 0.0f));
    cubeModel.setScale(vec3(1.0f));
    cubeModel.draw(cmd, &testPipeline);

    fbo.endRenderPass(cmd);
    device->endSingleTimeCommands(cmd);

    fbo.saveColorImageToPNG("fbo_debug.png");
}*/