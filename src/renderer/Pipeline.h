#ifndef PIPELINE_H
#define PIPELINE_H

#include <optional>
#include <string>
#include "AttribType.h"
#include <vector>

class UniformBuffer;
class VulkanDevice;

class Pipeline {

    public:
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
    struct PipelineConfig {

        int multisamplerSamples = 0;
        std::vector<ImageBinding> images;
        std::vector<UniformBinding> uniformObjects;

        std::string shaderName = "default";

        std::vector<AttribType::INPUT_TYPES> vertexAttributes;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
        VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
        float lineWidth = 1.0f;

        int pushConstantsSize = 0;
        bool depthTestEnable = true;
        bool depthWriteEnable = true;
        bool sampleShadding = false;
    };

    VkShaderModule createShaderModule(const std::vector<char> &code);
    bool loadShader(std::string shaderName, VkShaderModule &shaderModule, std::string shaderType);
    int recreate();

    Pipeline(VulkanDevice* vulkanDevice, VkRenderPass renderPass, PipelineConfig& config,
             VkDescriptorPool descriptorPool);

    /** @brief Recrea el pipeline entero. Si newConfig no viene, reusa la config actual.
     *  Si newRenderPass no viene (VK_NULL_HANDLE), reusa el render pass actual.
     *  @note Llamar SIEMPRE después de vkDeviceWaitIdle desde quien lo invoca. */
    void recreate(std::optional<PipelineConfig> newConfig = std::nullopt, VkRenderPass newRenderPass = VK_NULL_HANDLE);
    void destroyPipelineObjects();
    void buildPipeline();
    void updateDescriptorSet(std::vector<UniformBinding> uniformObjects, std::vector<ImageBinding> images);
    void updateShaders();
    ~Pipeline();
        bool error = false;
        void bind(VkCommandBuffer commandBuffer) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
        [[nodiscard]] const PipelineConfig& getConfig() const { return config; }
    void updateMSAASamples(int newValue);
    private:
        PipelineConfig config;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VulkanDevice* vulkanDevice = nullptr;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
};

#endif // PIPELINE_H