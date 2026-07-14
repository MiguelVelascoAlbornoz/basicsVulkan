#ifndef PIPELINE_H
#define PIPELINE_H

#include <Vulkan/vulkan.h>
#include <vector>
#include <string>
#include "VulkanDevice.h"

#define INPUT_TYPE_COUNT 9
#define DATA_COUNT_FROM_INPUT_TYPE 2
class Pipeline {
    struct VertexLayout {
    VkVertexInputBindingDescription binding;
    std::vector<VkVertexInputAttributeDescription> attributes;
    };
    /**
     * @brief La tabla relaciona el tipo de dato (float, int ,etc) con su tamaño en bytes y con su formato vulkan
     * tal que si haces attribsTable[CHAR][0] te devuelve el tamaño en bytes y attribsTable[CHAR][1] VK_FORMAT_R8_SINT
     * @note La tabla se inicializa en Pipeline.cpp
     */
    static int attribsTable[INPUT_TYPE_COUNT][DATA_COUNT_FROM_INPUT_TYPE];
    enum INPUT_TYPES {
        FLOAT,
        INT,
        CHAR,
        UINT,
        INT16,
        UINT16,
        UCHAR,
        VEC3,
        VEC4,
    };
    static size_t getSizeFromInputType(INPUT_TYPES input) {
        return attribsTable[input][0];
    }
    static VkFormat getFormatFromInputType(INPUT_TYPES input) {
        return static_cast<VkFormat>(attribsTable[input][1]);
    }

    public:
    /**
     * @brief A una dada lista de tipos de entrada (FLOAT, INT, VEC3, etc) le devuelve un VertexLayout que contiene el binding y los atributos con sus offsets y formatos.
       el binding contiene apenas informacion del stride encuanto los atributos cada atributo vendria siendo un layout.
       @details Construye los atributos iterando por cada input del vector dado y creando el vKVertexInputAttributeDescription
       obteniendo su formato y su tamaño apartir de la tabla.
     */
        VertexLayout getVertexLayoutFromInputs(std::vector<INPUT_TYPES> inputs);
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
        Pipeline(VulkanDevice* vulkanDevice, VkRenderPass renderPass, std::string shaderName);
        ~Pipeline();
        bool error = false;
        void bind(VkCommandBuffer commandBuffer) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }
    private:
        VulkanDevice* vulkanDevice = NULL;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
};

#endif // PIPELINE_H