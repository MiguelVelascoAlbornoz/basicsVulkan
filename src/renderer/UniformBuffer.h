

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
    UniformBuffer(VulkanDevice* device, size_t bytesCount);

    UniformBuffer(VulkanDevice *device, std::vector<VertexLayout::INPUT_TYPES> inputs);

    void setRaw(size_t index, const void *data, size_t expectedSize);

    void setFloat(size_t index, float value);

    void setVec3(size_t index, const glm::vec3 &value);

    void setMat4(size_t index, const glm::mat4 &value);

    void sendData(void* data);

private:
    std::vector<UniformField> fields;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mappedMemory;
    VulkanDevice* device;
    int bytesCount;
};

#endif // UNIFORM_BUFFER_H
