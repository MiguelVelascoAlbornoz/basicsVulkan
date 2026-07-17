#include "VertexLayout.h"



VertexLayout::VertexLayout(std::vector<AttribType::INPUT_TYPES> inputs)
{
    int offset = 0;
   
    for (AttribType::INPUT_TYPES input : inputs) {
        VkVertexInputAttributeDescription attributeDescription{};
        AttribType attribType = AttribType::getFormatFromInputType(input);
        attributeDescription.binding  = 0; // Asumiendo un solo binding
        attributeDescription.location = static_cast<uint32_t>(attributes.size());
        attributeDescription.format   = attribType.format;
        attributeDescription.offset   = offset;
        offset += attribType.size; // Incrementar el offset según el tamaño del tipo de entrada
        attributes.push_back(attributeDescription);
    }
    binding.binding = 0;
    binding.stride = offset; // al terminar el loop, offset == stride total
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

   
}