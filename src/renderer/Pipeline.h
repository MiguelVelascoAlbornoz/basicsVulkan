#ifndef PIPELINE_H
#define PIPELINE_H


#include <string>
class VulkanDevice;

#include "AttribType.h"
#include <vector>

class Pipeline {
    

    public:
    struct PipelineConfig {
        struct BindingDesc {
            VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        };

        // en PipelineConfig, reemplaza bindingsCount por:
        std::vector<BindingDesc> bindings;
        std::string shaderName = "default"; /** @brief file with the main in folder assets/shaders/etc/shaderName.etc  **/

        std::vector<AttribType::INPUT_TYPES> vertexAttributes; /** @brief Vector with the vertex layout and the types (order matters)**/

        VkPrimitiveTopology topology =
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; /** @brief Triangles join de vertex on triangles for ex **/

        VkPolygonMode polygonMode =
            VK_POLYGON_MODE_FILL;

        VkCullModeFlags cullMode =
            VK_CULL_MODE_NONE;

        VkFrontFace frontFace =
            VK_FRONT_FACE_CLOCKWISE;

        float lineWidth = 1.0f;


        int pushConstantsSize = 0;
        // En PipelineConfig
        bool depthTestEnable = true;
        bool depthWriteEnable = true;
    };
        /**
     * @brief Wraps SPIR-V bytecode into a VkShaderModule so it can be used in a pipeline shader stage.
     * @param code The raw SPIR-V bytecode, read from a compiled .spv file.
     * @details Vulkan does not compile GLSL; this only packages bytecode that was already compiled offline (e.g. with glslc) into a Vulkan object. codeSize must be a multiple of 4, since SPIR-V is a stream of 32-bit words.
     * @return A valid VkShaderModule handle. Throws std::runtime_error if creation fails.
     */
        VkShaderModule createShaderModule( const std::vector<char> &code);
        /**
         * @brief Loads a shader from a file, compiles it to SPIR-V, and creates a Vulkan shader module.
         * ShaderType should be "vert" for vertex shaders or "frag" for fragment shaders. The shaderName should not include the file extension.
            * @param shaderName The base name of the shader file (without extension).
            * @param shaderModule Vulkan shader module handler
            * @param shaderType The type of shader ("vert" for vertex, "frag" for fragment).
         */
        bool loadShader(std::string shaderName, VkShaderModule &shaderModule, std::string shaderType);

    /**
         * @brief Creates the graphics pipeline, which bundles shader stages, vertex input, input assembly, viewport/scissor, rasterization, multisampling, color blending, and dynamic state into a single immutable object.
         * @details Steps:
         * 1. Load and wrap vertex/fragment SPIR-V shaders via createShaderModule().
         * 2. Configure vertex input and input assembly (topology).
         * 3. Configure viewport/scissor as dynamic state.
         * 4. Configure rasterizer, multisampling, and color blending state.
         * 5. Reference pipelineLayout and renderPass.
         * 6. vkCreateGraphicsPipelines().
         * 7. Destroy the shader modules (no longer needed once the pipeline is built).
         * @note Requires createPipelineLayout() and createRenderPass() to have run first.
         */
        Pipeline(VulkanDevice* vulkanDevice, VkRenderPass renderPass, PipelineConfig& config,
                 VkDescriptorPool descriptorPool);
        ~Pipeline();
        bool error = false;
        void bind(VkCommandBuffer commandBuffer) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet;
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; } /**< @brief Get the pipeline layout handle. */
    private:

        VulkanDevice* vulkanDevice = nullptr;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
};

#endif // PIPELINE_H