
#include "Renderer.h"
#include <iostream>
#include <imGUI\imgui.h>
#include <imGUI\imgui_impl_sdl3.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>

bool Renderer::pickPhysicalDevice() {
    
    // Implementación de selección de dispositivo físico
    /**1. Enumeration of available GPUs in the system */
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    #ifdef _DEBUG
    std::cout << "(VULKAN) Available physical devices:" << std::endl;
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << " - " << deviceProperties.deviceName << std::endl;
    }
    #endif

    /**
     *2. Select one GPU device:
     @note this system has a dedicated GPU and an integrated GPU, we prefer the dedicated one.
     */
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        // Preferir GPU dedicada sobre integrada
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
          physicalDevice = device;
          break;
        }
    }
    //No se encontró una GPU dedicada, seleccionar la primera disponible
    if (physicalDevice == VK_NULL_HANDLE) {
        if (!devices.empty()) {
            physicalDevice = devices[0];
        } else {
            std::cerr << "(VULKAN) Error in pickPhysicalDevive(): No se encontraron dispositivos físicos compatibles con Vulkan." << std::endl;
            return false; // Retorna false si no se encuentra un dispositivo físico
        }
    }
    #ifdef _DEBUG
    VkPhysicalDeviceProperties selectedProps{};
    vkGetPhysicalDeviceProperties(physicalDevice, &selectedProps);
    std::cout << "(VULKAN) Selected physical device: " << selectedProps.deviceName << std::endl;
    std::cout << "(VULKAN) Device type: " << (selectedProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" : "Integrated GPU") << std::endl;
    std::cout << "(VULKAN) API version: " << VK_VERSION_MAJOR(selectedProps.apiVersion) << "." << VK_VERSION_MINOR(selectedProps.apiVersion) << "." << VK_VERSION_PATCH(selectedProps.apiVersion) << std::endl;
    std::cout << "(VULKAN) Driver version: " << selectedProps.driverVersion << std::endl;
    std::cout << "(VULKAN) Vendor ID: " << selectedProps.vendorID << std::endl;
    std::cout << "(VULKAN) Device ID: " << selectedProps.deviceID << std::endl;
    std::cout << "(VULKAN) Max image dimension 2D: " << selectedProps.limits.maxImageDimension2D << std::endl;
    std::cout << "(VULKAN) Max uniform buffer range: " << selectedProps.limits.maxUniformBufferRange << std::endl;
    std::cout << "(VULKAN) Max storage buffer range: " << selectedProps.limits.maxStorageBufferRange << std::endl;
    std::cout << "(VULKAN) Max push constants size: " << selectedProps.limits.maxPushConstantsSize << std::endl;
    std::cout << "(VULKAN) Max memory allocation count: " << selectedProps.limits.maxMemoryAllocationCount << std::endl;
    std::cout << "(VULKAN) Max sampler allocation count: " << selectedProps.limits.maxSamplerAllocationCount << std::endl;
    
    #endif
    return true; // Retorna true si se selecciona un dispositivo físico correctamente
}
bool Renderer::createLogicalDevice()
{
    uint32_t graphicsFamily = -1;
    uint32_t presentFamily = -1;
    /**
     * Get the queu family of the selected physical device
     */
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, families.data());


    for (uint32_t i = 0; i < families.size(); i++) {
        // Verificar graphics
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
        }

        // Verificar present para este mismo índice
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            presentFamily = i;
        }

        // Si ya tenemos ambos, salir
        if (graphicsFamily != 0xFFFFFFFF && presentFamily != 0xFFFFFFFF) break;
    }

    // Ahora sí verificar errores
    if (graphicsFamily == 0xFFFFFFFF) {
        SDL_Log("(VULKAN) Error in createLogicalDevice(): No se encontró familia de colas con soporte de gráficos.");
        return false;
    }
    if (presentFamily == 0xFFFFFFFF) {
        SDL_Log("(VULKAN) Error in createLogicalDevice(): No se encontró familia de colas con soporte de presentación.");
        return false;
    }
    if (presentFamily != graphicsFamily) {
        SDL_Log("(VULKAN) Error in createLogicalDevice(): La familia de colas de gráficos y presentación no coincide.");
        return false;
    }
    
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily; // índice obtenido en pickPhysicalDevice
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = queueCreateInfo.queueCount;
    createInfo.pQueueCreateInfos       = &queueCreateInfo;
    createInfo.enabledExtensionCount   = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error in createLogicalDevice(): No se pudo crear el dispositivo lógico.");
       return false;
    } else {
        #ifdef _DEBUG
        std::cout << "(VULKAN) Dispositivo lógico creado correctamente." << std::endl;
        #endif
    }
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);

    // Guardar índices para el swapchain después
    this->graphicsQueueFamilyIndex = graphicsFamily;
    this->presentQueueFamilyIndex  = presentFamily;

    return true;
}

bool Renderer::createSwapchain(Window *window)
{
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
        SDL_Log("(VULKAN) Error in createSwapChain(): No se encontraron formatos de superficie compatibles.");
        return false;
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
        SDL_Log("(VULKAN) Error in createSwapChain(): No se pudo crear el swapchain.");
        return false;
    }
        // Obtener las imágenes del swapchain
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    // Guardar formato y extent para usarlos después
    swapchainFormat = chosenFormat.format;
    swapchainExtent = extent;

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Swapchain creado: %dx%d, %d imagenes", extent.width, extent.height, imageCount);
    #endif

    return true;
}

bool Renderer::createRenderPass()
{
    // Descripción del color attachment (la imagen de la swapchain)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapchainFormat;
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error in createRenderPass(): No se pudo crear el render pass.");
        return false;
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Render pass creado correctamente.");
    #endif

    return true;
}

bool Renderer::createImageViews()
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
VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("createShaderModule(): No se pudo crear el shader module.");
    }
    return shaderModule;
}
bool Renderer::createFramebuffers()
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
bool Renderer::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex; // el índice que guardaste al elegir el physical device

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: No se pudo crear el command pool.");
        return false;
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Command pool creado correctamente.");
    #endif

    return true;
}
bool Renderer::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: No se pudieron crear los command buffers.");
        return false;
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) %d command buffers creados correctamente.", MAX_FRAMES_IN_FLIGHT);
    #endif

    return true;
}
bool Renderer::createSyncObjects()
{
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