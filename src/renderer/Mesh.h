#ifndef MESH_H
#define MESH_H

#include "VulkanDevice.h"

class Mesh {
public:
    Mesh();
    Mesh(VulkanDevice *device, const std::vector<char> &vertices, const std::vector<uint32_t> &indices);
    ~Mesh();
    void createVertexBuffer(const std::vector<char> &data);

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