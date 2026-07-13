#include "SwapChain.h"
#include <iostream>
#include <algorithm>
#include "Window.h"
SwapChain::SwapChain(VkPhysicalDevice physicalDevice,VkDevice device,Window* window)
{
    this->device = device;
    VkSurfaceKHR surface = window->surface;
    // Capacidades de la surface
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    //Ver los formatos soportados
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
    
    // Modos de presentación soportados
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    #ifdef _DEBUG
    std::cout << "(VULKAN) Supported surface formats:" << std::endl;
    for (const auto& format : formats) {
        std::cout << " - Format: " << format.format << ", Color Space: " << format.colorSpace << std::endl;
    }
    std::cout << "(VULKAN) Supported present modes:" << std::endl;
    for (const auto& mode : presentModes) {
        std::cout << " - Present Mode: " << mode << std::endl;
    }
    #endif

    // Elegir formato (preferir BGRA8 con SRGB)
    if (formats.empty()) {
        std::cout << "(VULKAN) Error in createSwapChain(): No se encontraron formatos de superficie compatibles." << std::endl;
        error =  true;
        return;
    }
    VkSurfaceFormatKHR chosenFormat = formats[0]; // fallback
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = f;
            break;
        }
    }

    // Elegir modo de presentación (preferir MAILBOX, fallback FIFO)
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // siempre disponible
    for (const auto& m : presentModes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = m;
            break;
        }
    }

    // Tamaño del swapchain
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent = {
            (uint32_t)window->getWidth(),
            (uint32_t)window->getHeight()
        };
        extent.width  = std::clamp(extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
     // Número de imágenes (triple buffering si es posible)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = chosenFormat.format;
    createInfo.imageColorSpace  = chosenFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // graphics y present son la misma familia
    createInfo.preTransform     = capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = chosenPresentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        std::cout << "(VULKAN) Error in createSwapChain(): No se pudo crear el swapchain." << std::endl;
        error = true;
        return;
    }
        // Obtener las imágenes del swapchain
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    // Guardar formato y extent para usarlos después
    swapchainFormat = chosenFormat.format;
    swapchainExtent = extent;

    #ifdef _DEBUG
    std::cout << "(VULKAN) Swapchain creado: " << extent.width << "x" << extent.height << ", " << imageCount << " imagenes" << std::endl;
    #endif

    bool imageViewsResult = createImageViews();
    if (!imageViewsResult){
        error = true;
    }
}
SwapChain::~SwapChain()
{
    // 8. Image views del swapchain
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // 9. Swapchain
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}
bool SwapChain::createImageViews()
{
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image    = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format   = swapchainFormat;

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
bool SwapChain::createFramebuffers(VkRenderPass renderPass)
{
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
void SwapChain::destroyFrameBuffers(VkDevice device) {
    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapchainFramebuffers.clear();
}


bool SwapChain::acquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t *pImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphore, VK_NULL_HANDLE, pImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "(VULKAN) Error in acquireNextImage(): No se pudo adquirir la siguiente imagen del swapchain." << std::endl;
        return false;
    }
    return true;
}

bool SwapChain::presentImage(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore* renderFinishedSemaphore)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // recreateSwapchain(); // implementar más adelante
        return false;
    } else if (result != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al presentar la imagen.");
        return false;
    }
    return true;
}
