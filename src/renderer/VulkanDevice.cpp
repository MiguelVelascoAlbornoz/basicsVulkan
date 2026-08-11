#include "VulkanDevice.h"
#include <iostream>



VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
{
    if (!pickPhysicalDevice(instance)) {
        error = true;
        return;
    } 
    if (!createLogicalDevice(surface)) {
        error = true;
    }
}

VulkanDevice::~VulkanDevice()
{    // 4. Command pool (destruye automáticamente los command buffers alocados de él)
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }
    // 12. Device (después de TODO lo que dependía de él)
    if (device != VK_NULL_HANDLE) {
        try {vkDestroyDevice(device, nullptr);} catch (...) {}

    }
}
// Agregar a VulkanDevice
void VulkanDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(cmd);
}
ImGui_ImplVulkan_InitInfo VulkanDevice::getImGuiInfo(VkDescriptorPool& imguiDescriptorPool, VkInstance instance, int imageCount) const {
    // Descriptor pool dedicado para ImGui
    VkDescriptorPoolSize poolSizes[] = { { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 100 } };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 100;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = poolSizes;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool);

    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance            = instance;
    info.PhysicalDevice      = physicalDevice;
    info.Device              = device;
    info.QueueFamily         = graphicsQueueFamilyIndex;
    info.Queue               = graphicsQueue;
    info.DescriptorPool      = imguiDescriptorPool;
    info.MinImageCount       = 2;
    info.ImageCount          = imageCount;
    info.Allocator           = nullptr;
    info.CheckVkResultFn     = nullptr;

    return info;

}

bool VulkanDevice::pickPhysicalDevice(VkInstance instance)
{

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
    std::cout << "Max dispatch X: " <<selectedProps. limits.maxComputeWorkGroupCount[0] << '\n';

    std::cout << "Max dispatch Y: "
              << selectedProps.limits.maxComputeWorkGroupCount[1] << '\n';

    std::cout << "Max dispatch Z: "
              << selectedProps.limits.maxComputeWorkGroupCount[2] << '\n';

    std::cout << "Max invocations/workgroup: "
              << selectedProps.limits.maxComputeWorkGroupInvocations << '\n';

    std::cout << "Max local size X: "
              << selectedProps.limits.maxComputeWorkGroupSize[0] << '\n';

    std::cout << "Max local size Y: "
              <<selectedProps. limits.maxComputeWorkGroupSize[1] << '\n';

    std::cout << "Max local size Z: "
              << selectedProps.limits.maxComputeWorkGroupSize[2] << '\n';

    std::cout << "Max shared memory: "
              << selectedProps.limits.maxComputeSharedMemorySize << " bytes\n";
    
    #endif
    return true; // Retorna true si se selecciona un dispositivo físico correctamente
}
bool VulkanDevice::createLogicalDevice(VkSurfaceKHR surface)
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
        std::cerr << "(VULKAN) Error in createLogicalDevice(): No se encontró familia de colas con soporte de gráficos." << std::endl;
        return false;
            return false;
    }
    if (presentFamily == 0xFFFFFFFF) {
        std::cerr << "(VULKAN) Error in createLogicalDevice(): No se encontró familia de colas con soporte de presentación." << std::endl;
        return false;
    }
    if (presentFamily != graphicsFamily) {
        std::cerr << "(VULKAN) Error in createLogicalDevice(): La familia de colas de gráficos y presentación no coincide." << std::endl;
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
    VkPhysicalDeviceFeatures deviceFeatures{}; // <-- nuevo
    deviceFeatures.sampleRateShading = VK_TRUE; // <-- nuevo, necesario para sampleShadingEnable
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = queueCreateInfo.queueCount;
    createInfo.pQueueCreateInfos       = &queueCreateInfo;
    createInfo.enabledExtensionCount   = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;



     // <-- nuevo

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        std::cerr << "(VULKAN) Error in createLogicalDevice(): No se pudo crear el dispositivo lógico." << std::endl;
       return false;
    }
        #ifdef _DEBUG
        std::cout << "(VULKAN) Dispositivo lógico creado correctamente." << std::endl;
        #endif

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);

    // Guardar índices para el swapchain después
    this->graphicsQueueFamilyIndex = graphicsFamily;
    this->presentQueueFamilyIndex  = presentFamily;

    return true;
}

bool VulkanDevice::queueSubmit(const VkSubmitInfo *submitInfo, VkFence fence)
{
     if ( vkQueueSubmit(graphicsQueue, 1, submitInfo, fence) != VK_SUCCESS) {
        std::cerr << "(VULKAN) Error in queueSubmit(): No se pudo enviar el command buffer a la cola." << std::endl;
        return false;
     } 
     return true;
}

bool VulkanDevice::presentKHR(VkPresentInfoKHR *presentInfo)
{
    const VkResult result = vkQueuePresentKHR(presentQueue, presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // recreateSwapchain(); // implementar más adelante
        return false;
    }
    if (result != VK_SUCCESS) {
        std::cerr << "(VULKAN) Error in presentKHR(): fallo al presentar la imagen." << std::endl;
        return false;
    }
    return true;
}
bool VulkanDevice::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex; // el índice que guardaste al elegir el physical device
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cerr << "(VULKAN) Error in createCommandPool(): No se pudo crear el command pool." << std::endl;
        return false;
    }

    #ifdef _DEBUG
    std::cout << "(VULKAN) Command pool creado correctamente." << std::endl;
    #endif

    return true;
}
bool VulkanDevice::createCommandBuffers(std::vector<VkCommandBuffer> &commandBuffers)
{
    
    commandBuffers.resize(2);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "(VULKAN) Error in createCommandBuffers(): No se pudo asignar los command buffers." << std::endl;
        return false;
    }

    #ifdef _DEBUG
    std::cout << "(VULKAN) " << commandBuffers.size() << " command buffers creados correctamente." << std::endl;
    #endif

    return true;
}
// VulkanDevice.cpp

void VulkanDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags properties,
                                  VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    // 1. Crear el VkBuffer (esto solo describe el buffer, no reserva memoria todavía)
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("No se pudo crear el buffer");
    }

    // 2. Preguntar qué memoria necesita ese buffer (tamaño real, alineación, tipos compatibles)
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // 3. Alocar la memoria del tipo correcto (host visible, device local, etc)
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("No se pudo alocar memoria del buffer");
    }

    // 4. Asociar la memoria alocada con el buffer creado
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{


    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        const bool isSupported = typeFilter & (1 << i);
        bool hasProperties = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;

        if (isSupported && hasProperties) {
            return i;
        }
    }

    throw std::runtime_error("No se encontró un tipo de memoria adecuado");
}
void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
VkCommandBuffer VulkanDevice::beginSingleTimeCommands() {
    // 1. Alocar un command buffer temporal del pool existente
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 2. Empezar a grabar, indicando que se va a usar UNA sola vez
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue); // espera a que la GPU termine antes de continuar

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}