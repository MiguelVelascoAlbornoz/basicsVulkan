#ifndef PIPELINE_H
#define PIPELINE_H

#include <Vulkan/vulkan.h>
#include <vector>
#include <string>

class Pipeline {

    public:
        VkShaderModule createShaderModule( const std::vector<char> &code);
        /**
         * @brief Loads a shader from a file, compiles it to SPIR-V, and creates a Vulkan shader module.
         * ShaderType should be "vert" for vertex shaders or "frag" for fragment shaders. The shaderName should not include the file extension.
            * @param shaderName The base name of the shader file (without extension).
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
        Pipeline(VkDevice device, VkRenderPass renderPass, std::string shaderName);
        ~Pipeline();
        VkDevice device = VK_NULL_HANDLE;
        bool error = false;
        void bind(VkCommandBuffer commandBuffer) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }
    private:
        
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
};

#endif // PIPELINE_H