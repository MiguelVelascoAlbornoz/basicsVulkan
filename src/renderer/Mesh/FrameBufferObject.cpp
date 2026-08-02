#include "FrameBufferObject.h"
#include "../VulkanDevice.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <stbImage/stb_image_write.h>

FrameBufferObject::FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat colorFormat)
    : device(device), width(width), height(height), colorFormat(colorFormat)
{
    createColorResources();
    if (error) return;
    createRenderPass();
    if (error) return;
    createFramebuffer();
}

FrameBufferObject::~FrameBufferObject()
{
    VkDevice dev = device->device;
    if (framebuffer != VK_NULL_HANDLE)     vkDestroyFramebuffer(dev, framebuffer, nullptr);
    if (renderPass != VK_NULL_HANDLE)      vkDestroyRenderPass(dev, renderPass, nullptr);
    if (colorImageView != VK_NULL_HANDLE)  vkDestroyImageView(dev, colorImageView, nullptr);
    if (colorImage != VK_NULL_HANDLE)      vkDestroyImage(dev, colorImage, nullptr);
    if (colorImageMemory != VK_NULL_HANDLE) vkFreeMemory(dev, colorImageMemory, nullptr);
}

void FrameBufferObject::createColorResources()
{
    VkDevice dev = device->device;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = colorFormat;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // COLOR_ATTACHMENT para renderizar, TRANSFER_SRC para poder copiarla despues a un buffer (debug PNG)
    imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(dev, &imageInfo, nullptr, &colorImage) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear la imagen de color." << std::endl;
        error = true;
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(dev, colorImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &colorImageMemory) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo alocar memoria para la imagen de color." << std::endl;
        error = true;
        return;
    }
    vkBindImageMemory(dev, colorImage, colorImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = colorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = colorFormat;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(dev, &viewInfo, nullptr, &colorImageView) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el image view de color." << std::endl;
        error = true;
    }
}

void FrameBufferObject::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = colorFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    // La dejamos en TRANSFER_SRC_OPTIMAL, asi al terminar el render pass ya esta lista para copiarla
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el render pass." << std::endl;
        error = true;
    }
}

void FrameBufferObject::createFramebuffer()
{
    VkImageView attachments[] = { colorImageView };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el framebuffer." << std::endl;
        error = true;
    }
}

void FrameBufferObject::beginRenderPass(VkCommandBuffer cmd)
{
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = { width, height };
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width    = static_cast<float>(width);
    viewport.height   = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = { width, height };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void FrameBufferObject::endRenderPass(VkCommandBuffer cmd)
{
    vkCmdEndRenderPass(cmd);
}

void FrameBufferObject::saveColorImageToPNG(const std::string& filename)
{
    VkDevice dev = device->device;

    // El render pass ya deja la imagen en TRANSFER_SRC_OPTIMAL (finalLayout),
    // asi que podemos copiarla directo a un buffer de staging sin barrier manual.
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4; // RGBA8 = 4 bytes/pixel

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    device->createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    VkCommandBuffer cmd = device->beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };

    vkCmdCopyImageToBuffer(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    device->endSingleTimeCommands(cmd); // hace submit + vkQueueWaitIdle, asi que al volver ya termino

    void* data;
    vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &data);
    std::vector<unsigned char> pixels(imageSize);
    memcpy(pixels.data(), data, imageSize);
    vkUnmapMemory(dev, stagingMemory);

    // Si usaras un formato BGRA (comun en swapchains) habria que invertir R y B aqui:
    if (colorFormat == VK_FORMAT_B8G8R8A8_UNORM || colorFormat == VK_FORMAT_B8G8R8A8_SRGB) {
        for (size_t i = 0; i < pixels.size(); i += 4) {
            std::swap(pixels[i], pixels[i + 2]);
        }
    }

    stbi_write_png(filename.c_str(), static_cast<int>(width), static_cast<int>(height),
                    4, pixels.data(), static_cast<int>(width) * 4);

    vkDestroyBuffer(dev, stagingBuffer, nullptr);
    vkFreeMemory(dev, stagingMemory, nullptr);

    std::cout << "(FBO) Imagen guardada en: " << filename << std::endl;
}