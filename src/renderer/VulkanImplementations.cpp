
#include "Renderer.h"
#include <iostream>
#include <imGUI\imgui.h>
#include <imGUI\imgui_impl_sdl3.h>
#include <imGUI\imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL_vulkan.h>

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
            std::cerr << "(VULKAN) Error: No se encontraron dispositivos físicos compatibles con Vulkan." << std::endl;
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
        SDL_Log("(VULKAN) Error: No se encontró familia de colas con soporte de gráficos.");
        return false;
    }
    if (presentFamily == 0xFFFFFFFF) {
        SDL_Log("(VULKAN) Error: No se encontró familia de colas con soporte de presentación.");
        return false;
    }
    if (presentFamily != graphicsFamily) {
        SDL_Log("(VULKAN) Error: La familia de colas de gráficos y presentación no coincide.");
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
        SDL_Log("(VULKAN) Error: No se pudo crear el dispositivo lógico.");
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
    window->getWidth();
    return true;
}

bool Renderer::createRenderPass()
{
    return true;
}

bool Renderer::createPipeline()
{
    return true;
}
