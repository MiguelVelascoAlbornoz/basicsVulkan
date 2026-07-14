
#include "Renderer.h"
#include <iostream>
#include <imGUI\imgui.h>
#include <imGUI\imgui_impl_sdl3.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>


bool Renderer::createVulkanInstance(){

    /**
     * Create Vulkan instance
     */
        // Extensions necesarias para SDL3
    Uint32 extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    #ifdef _DEBUG
        std::cout << "(VULKAN) Extensions:" << std::endl;
    for (unsigned int i = 0; i < extensionCount; ++i) {
        std::cout << " - " << extensions[i] << std::endl;
    }
    #endif

    VkApplicationInfo appInfo{};
    appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = PROJECT_NAME;
    //appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "BasicsVulkan";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_SUCCESS) {
        #ifdef _DEBUG
        std::cout << "(VULKAN) Vulkan está disponible!\n";
        #endif
    } else {
        std::cout << "(VULKAN) Vulkan no está disponible. Código: " << result << "\n";
        return false;
    }
    return true;
}

bool Renderer::createRenderPass()
{
    // Descripción del color attachment (la imagen de la swapchain)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChain->getSwapchainImageFormat();
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Referencia al attachment desde la subpass
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Definición de la subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    // Dependencia: esperar a que la imagen esté disponible antes de escribir en ella
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Crear el render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    VkDevice device = vulkanDevice->getDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error in createRenderPass(): No se pudo crear el render pass.");
        return false;
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Render pass creado correctamente.");
    #endif

    return true;
}




bool Renderer::createSyncObjects()
{
    VkDevice device = vulkanDevice->getDevice();
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // arranca "señalado" para no bloquear el primer frame

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        bool ok = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) == VK_SUCCESS
               && vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) == VK_SUCCESS
               && vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) == VK_SUCCESS;

        if (!ok) {
            SDL_Log("(VULKAN) Error: No se pudieron crear los objetos de sincronizacion del frame %d.", i);
            return false;
        }
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Objetos de sincronizacion creados correctamente.");
    #endif

    return true;
}