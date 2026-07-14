#include "VertexLayout.h"

int VertexLayout::attribsTable[INPUT_TYPE_COUNT][DATA_COUNT_FROM_INPUT_TYPE] = {
    { sizeof(float), VK_FORMAT_R32_SFLOAT, 4 },          // FLOAT
    { sizeof(int), VK_FORMAT_R32_SINT, 4 },              // INT
    { sizeof(char), VK_FORMAT_R8_SINT, 4 },              // CHAR
    { sizeof(unsigned int), VK_FORMAT_R32_UINT, 4 },     // UINT
    { sizeof(int16_t), VK_FORMAT_R16_SINT, 4 },          // INT16
    { sizeof(uint16_t), VK_FORMAT_R16_UINT, 4 },         // UINT16
    { sizeof(unsigned char), VK_FORMAT_R8_UINT, 4 },     // UCHAR
    { 3 * sizeof(float), VK_FORMAT_R32G32B32_SFLOAT, 16 },// VEC3
    { 4 * sizeof(float), VK_FORMAT_R32G32B32A32_SFLOAT, 16}, // VEC4
    { 16 * sizeof(float), VK_FORMAT_R32G32B32A32_SFLOAT, 16 } // MAT4
};


VertexLayout::VertexLayout(std::vector<VertexLayout::INPUT_TYPES> inputs)
{
    int offset = 0;
   
    for (INPUT_TYPES input : inputs) {
        VkVertexInputAttributeDescription attributeDescription{};

        attributeDescription.binding  = 0; // Asumiendo un solo binding
        attributeDescription.location = static_cast<uint32_t>(attributes.size());
        attributeDescription.format   = getFormatFromInputType(input);
        attributeDescription.offset   = offset;
        offset += getSizeFromInputType(input); // Incrementar el offset según el tamaño del tipo de entrada
        attributes.push_back(attributeDescription);
    }
    binding.binding = 0;
    binding.stride = offset; // al terminar el loop, offset == stride total
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

   
}