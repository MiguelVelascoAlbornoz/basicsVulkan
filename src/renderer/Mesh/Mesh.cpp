#include "Mesh.h"

#include <cstring>
#include "../VulkanDevice.h"
Mesh::Mesh(VulkanDevice* device, void* data, size_t dataSize, int vertexCount, const std::vector<uint32_t>& indices)
    : device(device)
{
    createVertexBuffer(data,dataSize,vertexCount);
    createIndexBuffer(indices);
}

Mesh::~Mesh() {
    vkDestroyBuffer(device->device, indexBuffer, nullptr);
    vkFreeMemory(device->device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device->device, vertexBuffer, nullptr);
    vkFreeMemory(device->device, vertexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer(const void* data, const size_t dataSize, const int vertexCount) {
    const VkDeviceSize bufferSize = dataSize*vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device->createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* mappedMemory;
    vkMapMemory(device->device, stagingMemory, 0, bufferSize, 0, &mappedMemory);
    memcpy(mappedMemory, data,  bufferSize); // "vertices" es tu std::vector<Vertex>
    vkUnmapMemory(device->device, stagingMemory);

    device->createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer, vertexBufferMemory);

    device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device->device, stagingBuffer, nullptr);
    vkFreeMemory(device->device, stagingMemory, nullptr);
}

void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    const VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device->createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device->device, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(device->device, stagingMemory);

    device->createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer, indexBufferMemory);

    device->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device->device, stagingBuffer, nullptr);
    vkFreeMemory(device->device, stagingMemory, nullptr);
}

void Mesh::bind(const VkCommandBuffer cmd) const {
    const VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer cmd) const {
    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
}