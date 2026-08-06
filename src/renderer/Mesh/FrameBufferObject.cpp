#include "FrameBufferObject.h"
#include "../VulkanDevice.h"
#include "../Pipeline.h"   // <-- nuevo
#include <iostream>
#include <vector>
#include <array>           // <-- nuevo (para las 2 dependencies)
#include <cstring>
#include <stbImage/stb_image_write.h>
#include <algorithm>
#include <cmath>

FrameBufferObject::FrameBufferObject(VulkanDevice* device, uint32_t width, uint32_t height,
                                     VkFormat colorFormat, bool useDepth, bool createDepthSampler, int multiSamplerPower)
    : device(device), width(width), height(height), colorFormat(colorFormat), createDepthSampler(createDepthSampler), useDepth(useDepth), multiSamplerPower(multiSamplerPower)
{
    createColorResources();
    if (error) return;
    if (useDepth) {
        createDepthResources();
        if (error) return;
    }


    createResolveResources();
    
    createRenderPass();
    if (error) return;
    createFramebuffer();
}
void FrameBufferObject::createFramebuffer()
{
    std::vector attachments = { colorImageView };
    if (useDepth) {
        attachments.push_back(depthImageView);
    }
    if (multiSamplerPower > 0) {
        attachments.push_back(resolveColorImageView); // debe ir en el mismo orden que en createRenderPass
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el framebuffer." << std::endl;
        error = true;
    }
}

VkFormat FrameBufferObject::findDepthFormat() const
{
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->getPhysicalDevice(), format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    std::cerr << "(FBO) Error: no se encontro un formato de depth soportado." << std::endl;
    return VK_FORMAT_UNDEFINED;
}
void FrameBufferObject::createDepthResources()
{
    VkDevice dev = device->device;

    depthFormat = findDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED) {
        error = true;
        return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = depthFormat;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (createDepthSampler) imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = static_cast<VkSampleCountFlagBits>(std::pow(2,multiSamplerPower));
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(dev, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear la imagen de depth." << std::endl;
        error = true;
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(dev, depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo alocar memoria para la imagen de depth." << std::endl;
        error = true;
        return;
    }
    vkBindImageMemory(dev, depthImage, depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = depthFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (createDepthSampler && depthSampler == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR; // o NEAREST si prefieres depth sin filtrar
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // 1.0 = "infinitamente lejos" fuera de rango
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        vkCreateSampler(device->device, &samplerInfo, nullptr, &depthSampler);
    }
    if (vkCreateImageView(dev, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el image view de depth." << std::endl;
        error = true;
    }
}

void FrameBufferObject::addScene(SceneFunction scene)
{
    scenes.push_back(scene);
}

void FrameBufferObject::removeScene(SceneFunction scene)
{
    auto it = std::find(scenes.begin(),  scenes.end(), scene);

    if (it != scenes.end())
    {
        scenes.erase(it);
    } else
    {
        std::cerr << "Scene not active" << std::endl;
    }
}

void FrameBufferObject::renderScenes(VkCommandBuffer cmd)
{
    for (auto & scene : scenes)
    {
        scene(cmd);
    }
}
void FrameBufferObject::recreateFrameBuffer()
{
    // Solo recursos de imagen (color/depth/resolve), SIN framebuffer ni render pass
    VkDevice dev = device->device;
    if (framebuffer != VK_NULL_HANDLE)      { vkDestroyFramebuffer(dev, framebuffer, nullptr); framebuffer = VK_NULL_HANDLE; }
    if (depthImageView != VK_NULL_HANDLE)   vkDestroyImageView(dev, depthImageView, nullptr);
    if (depthImage != VK_NULL_HANDLE)       vkDestroyImage(dev, depthImage, nullptr);
    if (depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(dev, depthImageMemory, nullptr);
    if (colorImageView != VK_NULL_HANDLE)   vkDestroyImageView(dev, colorImageView, nullptr);
    if (colorImage != VK_NULL_HANDLE)       vkDestroyImage(dev, colorImage, nullptr);
    if (colorImageMemory != VK_NULL_HANDLE) vkFreeMemory(dev, colorImageMemory, nullptr);
    destroyResolveResources();

    createColorResources();
    if (error) return;
    if (useDepth) {
        createDepthResources();
        if (error) return;
    }
    createResolveResources();
    // NOTA: ya NO llama createFramebuffer() acá
}

void FrameBufferObject::changeResolution(int newWidth, int newHeight) {
    VkDevice dev = device->device;
    vkDeviceWaitIdle(dev);
    width = newWidth;
    height = newHeight;
    recreateFrameBuffer();          // recrea imágenes (mismo sample count, mismo renderPass)
    if (error) return;
    createFramebuffer();            // renderPass no cambió, se puede rehacer el framebuffer ya
}

void FrameBufferObject::changeMultiSamplerPower(int newMultiSamplerPower)
{
    VkDevice dev = device->device;
    vkDeviceWaitIdle(dev);

    multiSamplerPower = newMultiSamplerPower;

    // 1. Recrear imágenes (color/depth/resolve) según el nuevo sample count
    recreateFrameBuffer();
    if (error) return;

    // 2. Recrear el render pass CON el nuevo attachment count (antes que el framebuffer)
    if (renderPass != VK_NULL_HANDLE) { vkDestroyRenderPass(dev, renderPass, nullptr); renderPass = VK_NULL_HANDLE; }
    createRenderPass();
    if (error) return;

    // 3. Ahora sí, un único createFramebuffer, con imágenes Y render pass ya consistentes
    createFramebuffer();
    if (error) return;

    // 4. Cascada a los pipelines dependientes
    for (Pipeline* pipeline : dependentPipelines) {
        Pipeline::PipelineConfig cfg = pipeline->getConfig();
        cfg.multisamplerSamples = newMultiSamplerPower;
        pipeline->recreate(cfg, renderPass);
    }
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
    imageInfo.samples       = static_cast<VkSampleCountFlagBits>(std::pow(2,multiSamplerPower));;
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
    if (colorSampler ==VK_NULL_HANDLE) {
        // en createColorResources(), después de crear el imageView
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        vkCreateSampler(device->device, &samplerInfo, nullptr, &colorSampler);
    }
}

void FrameBufferObject::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachments;

    VkSampleCountFlagBits samples = static_cast<VkSampleCountFlagBits>(1 << multiSamplerPower);

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = colorFormat;
    colorAttachment.samples        = samples; // antes estaba hardcodeado a VK_SAMPLE_COUNT_1_BIT
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    // si hay resolve, esta imagen multisample no se samplea directo, solo la resolve;
    // si no hay MSAA, se comporta como antes
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments.push_back(colorAttachment);

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    if (useDepth) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format         = depthFormat;
        depthAttachment.samples        = samples; // debe coincidir con el color attachment
        depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // FIX: estaba invertido. Si vas a samplear el depth después (createDepthSampler=true),
        // hay que GUARDARLO (STORE), no descartarlo.
        depthAttachment.storeOp = createDepthSampler ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout    = createDepthSampler ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // ---- Resolve attachment: solo si hay MSAA ----
    VkAttachmentReference resolveAttachmentRef{};
    if (multiSamplerPower > 0) {
        VkAttachmentDescription resolveAttachment{};
        resolveAttachment.format         = colorFormat;
        resolveAttachment.samples        = VK_SAMPLE_COUNT_1_BIT; // siempre 1, es la imagen "final"
        resolveAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments.push_back(resolveAttachment);

        resolveAttachmentRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
        resolveAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr;
    subpass.pResolveAttachments = (multiSamplerPower > 0) ? &resolveAttachmentRef : nullptr;

    // Dependency de entrada (ya existía)
    VkSubpassDependency dependencyIn{};
    dependencyIn.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependencyIn.dstSubpass    = 0;
    dependencyIn.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencyIn.srcAccessMask = 0;
    dependencyIn.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencyIn.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Dependency de salida (nueva): garantiza que el post-proceso vea las escrituras completas
    VkSubpassDependency dependencyOut{};
    dependencyOut.srcSubpass    = 0;
    dependencyOut.dstSubpass    = VK_SUBPASS_EXTERNAL;
    dependencyOut.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencyOut.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencyOut.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencyOut.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    std::array<VkSubpassDependency, 2> dependencies = { dependencyIn, dependencyOut };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies   = dependencies.data();

    if (vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el render pass." << std::endl;
        error = true;
    }
}

void FrameBufferObject::beginRenderPass(VkCommandBuffer cmd)
{
    std::vector<VkClearValue> clearValues;
    VkClearValue clearColor{};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues.push_back(clearColor);

    if (useDepth) {
        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};
        clearValues.push_back(clearDepth);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = { width, height };
    renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

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
void FrameBufferObject::createResolveResources()
{
    if (multiSamplerPower == 0) return; // nada que hacer sin MSAA

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
    imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT; // resolve siempre es 1 sample
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(dev, &imageInfo, nullptr, &resolveColorImage) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear la imagen de resolve." << std::endl;
        error = true;
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(dev, resolveColorImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &resolveColorImageMemory) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo alocar memoria para la imagen de resolve." << std::endl;
        error = true;
        return;
    }
    vkBindImageMemory(dev, resolveColorImage, resolveColorImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = resolveColorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = colorFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(dev, &viewInfo, nullptr, &resolveColorImageView) != VK_SUCCESS) {
        std::cerr << "(FBO) Error: no se pudo crear el image view de resolve." << std::endl;
        error = true;
    }
}

FrameBufferObject::~FrameBufferObject()
{
    VkDevice dev = device->device;
    if (framebuffer != VK_NULL_HANDLE)      vkDestroyFramebuffer(dev, framebuffer, nullptr);
    if (renderPass != VK_NULL_HANDLE)       vkDestroyRenderPass(dev, renderPass, nullptr);
    if (colorSampler != VK_NULL_HANDLE)     vkDestroySampler(dev, colorSampler, nullptr);
    if (depthSampler != VK_NULL_HANDLE)     vkDestroySampler(dev, depthSampler, nullptr);
    if (depthImageView != VK_NULL_HANDLE)   vkDestroyImageView(dev, depthImageView, nullptr);
    if (depthImage != VK_NULL_HANDLE)       vkDestroyImage(dev, depthImage, nullptr);
    if (depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(dev, depthImageMemory, nullptr);
    if (colorImageView != VK_NULL_HANDLE)   vkDestroyImageView(dev, colorImageView, nullptr);
    if (colorImage != VK_NULL_HANDLE)       vkDestroyImage(dev, colorImage, nullptr);
    if (colorImageMemory != VK_NULL_HANDLE) vkFreeMemory(dev, colorImageMemory, nullptr);
    destroyResolveResources(); // <-- nuevo
}
void FrameBufferObject::destroyResolveResources()
{
    VkDevice dev = device->device;
    if (resolveColorImageView != VK_NULL_HANDLE) { vkDestroyImageView(dev, resolveColorImageView, nullptr); resolveColorImageView = VK_NULL_HANDLE; }
    if (resolveColorImage != VK_NULL_HANDLE)     { vkDestroyImage(dev, resolveColorImage, nullptr);         resolveColorImage = VK_NULL_HANDLE; }
    if (resolveColorImageMemory != VK_NULL_HANDLE) { vkFreeMemory(dev, resolveColorImageMemory, nullptr);   resolveColorImageMemory = VK_NULL_HANDLE; }
}
