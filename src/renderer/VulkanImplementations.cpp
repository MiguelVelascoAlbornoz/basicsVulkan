
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
        error = true;
        return;
    }
}
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error in createRenderPass(): No se pudo crear el render pass.");
        return false;
    }

    #ifdef _DEBUG
    SDL_Log("(VULKAN) Render pass creado correctamente.");
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