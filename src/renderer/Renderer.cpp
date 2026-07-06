/**
 * @file Renderer.cpp
 * @brief Renderer class implementation.
 * @author Miguel Velasco
 */
#include "Renderer.h"
#include <iostream>
#include <imGUI\imgui.h>
#include <imGUI\imgui_impl_sdl3.h>
#include <imGUI\imgui_impl_vulkan.h>
#include <SDL3/SDL_vulkan.h>


/**
 * @brief Initializes the GUI using ImGui with SDL3 and SDL_Renderer3 backends.
 * @details This function sets up the ImGui context, configures the IO settings, If any of the initialization steps fail, it sets the error flag in the Renderer class to true and prints an error message to the standard error stream.
 */
void Renderer::initGUI(Window* window)
{
    ImGuiContext* ctx = ImGui::CreateContext();
    if (!ctx) {
        std::cerr << "Error: No se pudo crear el contexto de ImGui in initGUI()." << std::endl;
        error = true;
        return;
    }   

    ImGui::StyleColorsDark();
   ImGui_ImplSDL3_InitForVulkan(window->window);
 
     // Descriptor pool dedicado para ImGui
    VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 } };
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
info.ImageCount          = static_cast<uint32_t>(swapchainImages.size());
info.Allocator           = nullptr;
info.CheckVkResultFn     = nullptr;


info.PipelineInfoMain.RenderPass   = renderPass;
info.PipelineInfoMain.Subpass      = 0;
info.PipelineInfoMain.MSAASamples  = VK_SAMPLE_COUNT_1_BIT;

    bool ok2 = ImGui_ImplVulkan_Init(&info);

    if ( !ok2) {
        std::cerr << "Error in initGUI(): ImGui no se inicializó correctamente in initGUI." << std::endl;
        error = true;
    } 
    #ifdef _DEBUG 
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    std::cout << "ImGui version: " << ImGui::GetVersion() << std::endl;
    std::cout << "Backend Renderer: " << (io.BackendRendererName ? io.BackendRendererName : "null") << std::endl;
    std::cout << "Backend Platform: " << (io.BackendPlatformName ? io.BackendPlatformName : "null") << std::endl;
    std::cout << "Display size: " << io.DisplaySize.x << " x " << io.DisplaySize.y << std::endl;
    
    #endif
}

void Renderer::initVulkan(Window* window)  {

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
    /**
     * Attach Vulkan surface to SDL window
     */
    if (!SDL_Vulkan_CreateSurface(window->window, instance, nullptr, &surface)) {
        std::cout << "(VULKAN) Failed to create surface in initVulkan(): " << SDL_GetError() << "\n";
        error = true;
        return;
    } else {
        #ifndef _DEBUG
        std::cout << "(VULKAN) Vulkan surface created successfully.\n";
        #endif
    }
    if (!pickPhysicalDevice() ||
     !createLogicalDevice() ||
      !createSwapchain(window) ||
       !createImageViews() ||
        !createRenderPass() ||
         !createPipeline() || 
          !createFramebuffers() ||
           !createCommandPool() ||
            !createCommandBuffers() ||
             !createSyncObjects()
        ) {
        error = true;
    }

}

Renderer::Renderer(Window *window)
{
    initVulkan(window);
    initGUI(window);

    
    #ifdef _DEBUG
    std::cout << "(SDL) Available render drivers:" << std::endl;
    int numDrivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < numDrivers; ++i) {
        const char* info = SDL_GetRenderDriver(i);
        std::cout << " - " << info << std::endl;
    }
    #endif
}

Renderer::~Renderer()
{
    //ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
   
    vkDestroyDevice(device, nullptr);         // antes que la instancia
    vkDestroySurfaceKHR(instance, surface, nullptr); // antes que la instancia
    vkDestroyInstance(instance, nullptr);     // último
}
/**
 * @brief Records the draw commands for a single frame into the given command buffer.
 * @param commandBuffer The command buffer to record into (already allocated, will be reset before recording).
 * @param imageIndex Index of the swapchain image/framebuffer to render into.
 */
void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al empezar a grabar el command buffer.");
        return;
    }

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} }; // negro

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Viewport y scissor son dinámicos en el pipeline, hay que setearlos cada frame
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)swapchainExtent.width;
    viewport.height   = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0); // triángulo fullscreen, sin vertex buffer

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al terminar de grabar el command buffer.");
    }
}

void Renderer::update(Menu* renderMenu)
{
    // 1. Preparar frame de ImGui (antes de tocar el command buffer)
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
  

    renderMenu->render();
    ImGui::Render();

    // Limpiar la pantalla
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // negro
    SDL_RenderClear(renderer);
   // 1. Esperar a que el frame anterior en este slot termine de usarse
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Adquirir la siguiente imagen disponible de la swapchain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) return;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        SDL_Log("(VULKAN) Error al adquirir imagen in update().");
        return;
    }

    // 3. Resetear el fence solo ahora que sabemos que vamos a enviar trabajo
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // 4. Grabar el command buffer de este frame
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    // 5. Enviar el command buffer a la cola de graphics
    VkSemaphore waitSemaphores[]      = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[]    = { renderFinishedSemaphores[currentFrame] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al enviar el command buffer a la cola.");
        return;
    }

    // 6. Presentar la imagen ya renderizada
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // recreateSwapchain(); // implementar más adelante
    } else if (result != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al presentar la imagen.");
    }

    // 7. Avanzar al siguiente frame en vuelo (round-robin entre los N slots)
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::updateGUI(Menu* menu)
{
    /**
     * @brief Starts a new ImGui frame for both SDL3 and SDL_Renderer3 backends. This function should be called at the beginning of each frame before submitting any ImGui UI commands. It prepares the ImGui context for a new frame, allowing you to create and submit your ImGui UI elements for rendering.
     */
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    menu->render();

    /**
     * @brief Finalizes the ImGui frame and renders the draw data. This function should be called after submitting all your ImGui UI commands for the current frame. It ends the ImGui frame, finalizes the draw data, and then calls the rendering function of the SDL_Renderer3 backend to render the ImGui elements on the screen.
     */
    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame], pipeline);
}
