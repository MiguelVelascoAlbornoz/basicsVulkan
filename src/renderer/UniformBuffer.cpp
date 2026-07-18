
#include "UniformBuffer.h"
#include <iostream>
#include <memory>

/**
 * @brief El constructor itera por cada input, obtiene el tamaño y el align de dado dato y calcula el offset al que se deve meter, despues inicailiza un UniformField que deve ser destruido con delete en el destructor
 * y lo mete en el vector de fields de este objeto. Tambien por defecto cada field empieza en el field queue, osea seran mandados al shader encuando se llame clearQueue().
 * Despues de haber generado el vector de rawData que se enviara al shader se crea el buffer y el mapMemory.
 * @details El uniform buffer deve estar alineado con el standart std140, osea, cada field deve comenzar en un multiplo de su propio align, si no lo es entonces el buffer se rellena con bytes redundantes hasta llegar al proximo multiplo del align del field.
 * Tambien el numero de bytes deve ser multiplo del maximo align usado en el uniform  buffer.
 * @param inputs vector de pares void*, INPUT_TYPE, osea, el puntero a la data que se desea mandar al shader y el tipo de data.
 * **/
UniformBuffer::UniformBuffer( VulkanDevice* device,std::vector<std::pair<const void*,AttribType::INPUT_TYPES>> &inputs){
    this->device = device;
    size_t offset = 0;
    size_t maxAlign = 0;
    for (auto& input : inputs) {
        const AttribType* attribType = AttribType::getAttribFromInputType(input.second);

        size_t size = attribType->size;
        size_t align = attribType->align;
        if (align > maxAlign) {
            maxAlign = align;
        }
        offset = (offset + align - 1) & ~(align - 1);; // Alinear el offset según el alineamiento requerido

        UniformField* field = new UniformField { .type = attribType->type, .offsetInUniform = offset, .size = size, .dataPointer = input.first };
        offset += size;
        fields.push_back(field);

        fieldToUpdateQueue.push_back(field);
    }
    bytesCount = static_cast<int>((offset + maxAlign - 1) & ~(maxAlign - 1)); // Alinear el tamaño total según el alineamiento máximo
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
void UniformBuffer::clearQueue() {
    while (const UniformField* field = getAndPopOfQueue()) {
        setRaw(field);
    }
}
const UniformBuffer::UniformField* UniformBuffer::getAndPopOfQueue(){
    if (!fieldToUpdateQueue.empty()) {
        const UniformField* retValue = fieldToUpdateQueue.back();
        fieldToUpdateQueue.pop_back();
        return retValue;
    } else {
        return nullptr;
    }
}

void UniformBuffer::setRaw(const UniformField* uniformField) {

    if (!uniformField->dataPointer) {
        void* tempMemory = calloc(1,uniformField->size);
        memcpy(static_cast<char*>(mappedMemory) + uniformField->offsetInUniform,tempMemory, uniformField->size);
        std::cout << "Error in setRaw(): Null pointer in uniform field. " << std::endl;
        std:: cout << "Data offset: " << uniformField->offsetInUniform << std::endl;
        std::cout << "Data type: " << uniformField->type << std::endl;
        free(tempMemory);
    } else {
        memcpy(static_cast<char*>(mappedMemory) + uniformField->offsetInUniform, uniformField->dataPointer, uniformField->size);
    }

}
void UniformBuffer::setRaw(size_t index) {
    if (index >= fields.size()) {
        std::cerr << "Índice de uniform fuera de rango: " << index << std::endl;
        return;
    }
    const UniformField* field = fields[index];
    setRaw(field);
}


void UniformBuffer::updateDescriptorSet(VulkanDevice* device, VkDescriptorSet descriptorSet)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bytesCount;
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;

    descriptorWrite.descriptorType =
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    descriptorWrite.descriptorCount = 1;

    descriptorWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(
    device->device,
    1,
    &descriptorWrite,
    0,
    nullptr
    );
}
