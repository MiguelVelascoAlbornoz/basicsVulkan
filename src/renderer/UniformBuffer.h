

#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "VertexLayout.h"
#include "VulkanDevice.h"
#include <glm/glm.hpp>

class UniformBuffer
{
public:
    struct UniformField {
        VertexLayout::INPUT_TYPES type;
        size_t offset;
        size_t size;
    };
    ~UniformBuffer() {
        vkUnmapMemory(device->device, memory);
        vkDestroyBuffer(device->device, buffer, nullptr);
        vkFreeMemory(device->device, memory, nullptr);
    }
    UniformBuffer(VulkanDevice *device, std::vector<VertexLayout::INPUT_TYPES> inputs);

    void setRaw(size_t index, const void *data, size_t expectedSize);

    void setFloat(size_t index, float value);

    void setVec3(size_t index, const glm::vec3 &value);

    void setMat4(size_t index, const glm::mat4 &value);

    void updateDescriptorSet(VulkanDevice *device, VkDescriptorSet descriptorSet);





private:
    std::vector<UniformField> fields;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mappedMemory;
    VulkanDevice* device;
    int bytesCount;
};

#endif // UNIFORM_BUFFER_H
