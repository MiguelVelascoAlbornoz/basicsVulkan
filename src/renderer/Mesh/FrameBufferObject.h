//
// Created by migue on 01/08/2026.
//

#ifndef BASICSVULKAN_FRAMEBUFFEROBJECT_H
#define BASICSVULKAN_FRAMEBUFFEROBJECT_H
#include "vulkan/vulkan_core.h"

class VulkanDevice;

class FrameBufferObject
{
public:
    struct BasicsAttachment
    {
        int channelsCount;
        int bytesPerChannel;
        bool depthStencil;
    };
    FrameBufferObject(VulkanDevice* device,int width, int height, int NAttachments, BasicsAttachment* attachments);

private:
    VulkanDevice* device;
    int width, height;
    int NAttachments;
    BasicsAttachment* attachments;
    void createRenderPass();
    VkRenderPass renderPass = VK_NULL_HANDLE;
};


#endif //BASICSVULKAN_FRAMEBUFFEROBJECT_H
