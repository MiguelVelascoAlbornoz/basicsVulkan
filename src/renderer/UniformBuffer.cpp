
#include "UniformBuffer.h"
#include <iostream>


UniformBuffer::UniformBuffer( VulkanDevice* device,std::vector<VertexLayout::INPUT_TYPES> inputs){
    this->device = device;
    size_t offset = 0;
    size_t maxAlign = 0;
    for (VertexLayout::INPUT_TYPES input : inputs) {
        size_t size = VertexLayout::getSizeFromInputType(input);
        size_t align = VertexLayout::getAlignFromInputType(input);
        if (align > maxAlign) {
            maxAlign = align;
        }
        offset = (offset + align - 1) & ~(align - 1);; // Alinear el offset según el alineamiento requerido
        UniformField field{ input, offset, size };
        offset += size;
        fields.push_back(field);
    }
    bytesCount = (offset + maxAlign - 1) & ~(maxAlign - 1); // Alinear el tamaño total según el alineamiento máximo
    device->createBuffer(
        bytesCount,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        memory
    );
    vkMapMemory(device->device, memory, 0, bytesCount, 0, &mappedMemory);
}

void UniformBuffer::setRaw(size_t index, const void* data, size_t expectedSize) {
    if (index >= fields.size()) {
        std::cerr << "Índice de uniform fuera de rango: " << index << std::endl;
        return;
    }
    const UniformField& field = fields[index];
    if (field.size != expectedSize) {
        std::cerr << "Tipo incorrecto para este campo de uniform. Se esperaba tamaño: " << field.size << ", pero se proporcionó tamaño: " << expectedSize << std::endl;
        return;
    }

    memcpy(static_cast<char*>(mappedMemory) + field.offset, data, expectedSize);
}

void UniformBuffer::setFloat(size_t index, float value) {
    setRaw(index, &value, sizeof(float));
}

void UniformBuffer::setVec3(size_t index, const glm::vec3& value) {
    setRaw(index, &value, sizeof(glm::vec3)); // sizeof(vec3) en glm = 12 bytes, coincide con std140 size
}

void UniformBuffer::setMat4(size_t index, const glm::mat4& value) {
    setRaw(index, &value, sizeof(glm::mat4)); // 64 bytes, coincide
}