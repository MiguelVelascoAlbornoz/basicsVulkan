#include "SwapChain.h"
#include <iostream>
#include <algorithm>
#include "Window.h"

SwapChain::SwapChain(VulkanDevice* vulkanDevice,Window* window,VkRenderPass renderPass,VkSurfaceFormatKHR chosenFormat )

{   

    this->vulkanDevice = vulkanDevice;
    VkPhysicalDevice physicalDevice = vulkanDevice->getPhysicalDevice();
    swapchainFormat = chosenFormat;
    surface = window->surface;
   
    
    // Modos de presentación soportados
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

   
    swapchainFormat = chosenFormat;
    // Elegir modo de presentación (preferir MAILBOX, fallback FIFO)
    presentMode = VK_PRESENT_MODE_FIFO_KHR; // siempre disponible
    for (const auto& m : presentModes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = m;
            break;
        }
    }


    if (!createSwapChain(window, renderPass)) {
        error = true;
        return;
    }
    
}

void SwapChain::destroySwapChain()
{
    // 5. Framebuffers (uno por imagen del swapchain)
    destroyFrameBuffers();

    // 8. Image views del swapchain
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(vulkanDevice->device, imageView, nullptr);
    }

    // 9. Swapchain
    vkDestroySwapchainKHR(vulkanDevice->device, swapchain, nullptr);
}

SwapChain::~SwapChain()
{
    destroySwapChain();
}
bool SwapChain::createImageViews()
{
    VkDevice device = vulkanDevice->device;

    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image    = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format   = swapchainFormat.format;

        // Sin swizzle, cada canal mapea a sí mismo
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Qué parte de la imagen describe esta vista
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            SDL_Log("(VULKAN) Error in createImageViews()");
            return false;
        }
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) image views creados correctamente.");
    #endif

    return true;
}
bool SwapChain::recreateSwapChain(VkRenderPass renderPass, Window* window)
{
    destroySwapChain();
    return createSwapChain(window, renderPass);
}
bool SwapChain::createFramebuffers(VkRenderPass renderPass)
{
    VkDevice device = vulkanDevice->device;
    // Un framebuffer por cada image view de la swapchain
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {

        VkImageView attachments[] = {
            swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;           // debe ser compatible con este render pass
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = swapchainExtent.width;
        framebufferInfo.height          = swapchainExtent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
            SDL_Log("(VULKAN) Error in createFrameBuffers(): No se pudo crear el framebuffer.");
            return false;
        }
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) framebuffers creados correctamente.");
    #endif

    
    return true;
}
bool SwapChain::createSwapChain(Window *window, VkRenderPass renderPass)
{
    // Capacidades de la surface
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanDevice->getPhysicalDevice(), surface, &capabilities);

    // Tamaño del swapchain
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent = {
            .width = static_cast<uint32_t>(window->getWidth()),
            .height = static_cast<uint32_t>(window->getHeight())
        };
        extent.width  = std::clamp(extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    uint32_t minImageCount = capabilities.minImageCount + 1; // +1 para triple buffering
    if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
        minImageCount = capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = minImageCount;
    createInfo.imageFormat      = swapchainFormat.format;
    createInfo.imageColorSpace  = swapchainFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // graphics y present son la misma familia
    createInfo.preTransform     = capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vulkanDevice->device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in createSwapChain(): No se pudo crear el swapchain." << std::endl;
    
        return false;
    }
    uint32_t imageCount = swapchainImageViews.size();
        // Obtener las imágenes del swapchain
    vkGetSwapchainImagesKHR(vulkanDevice->device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanDevice->device, swapchain, &imageCount, swapchainImages.data());

    // Guardar formato y extent para usarlos después

    swapchainExtent = extent;

    #ifdef _DEBUG
    std::cout << "(VULKAN) Swapchain creado: " << extent.width << "x" << extent.height << ", " << imageCount << " imagenes" << std::endl;
    #endif


    if (!createImageViews()){
        return false;
    }

    if (!createFramebuffers(renderPass)){
        return false;
    }
    return true;
}
void SwapChain::destroyFrameBuffers()
{
    for (const auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(vulkanDevice->device, framebuffer, nullptr);
    }
    swapchainFramebuffers.clear();
}

bool SwapChain::acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t *pImageIndex)
{
    VkDevice device = vulkanDevice->device;

    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphore, VK_NULL_HANDLE, pImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "(VULKAN) Error in acquireNextImage(): No se pudo adquirir la siguiente imagen del swapchain." << std::endl;
        return false;
    }
    return true;
}

bool SwapChain::presentImage(uint32_t imageIndex, VkSemaphore* renderFinishedSemaphore)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    if (!vulkanDevice->presentKHR(&presentInfo)) {
        return false;
    }
    return true;
}
