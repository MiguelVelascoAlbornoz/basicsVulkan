#ifndef MESH_H
#define MESH_H

#include "../VulkanDevice.h"

class Mesh {
public:

    Mesh(VulkanDevice *device, void *data, size_t dataSize, int vertexCount, const std::vector<uint32_t> &indices);
    ~Mesh();
    void createVertexBuffer(const void *data, size_t dataSize, int vertexCount);

    void createIndexBuffer(const std::vector<uint32_t> &indices);

    void bind(VkCommandBuffer cmd) const;

    void draw(VkCommandBuffer cmd) const;

private:
    VulkanDevice* device;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
};


#endif