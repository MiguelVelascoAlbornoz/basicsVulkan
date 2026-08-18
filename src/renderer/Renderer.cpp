/**
 * @file Renderer.cpp
 * @brief Renderer class implementation.
 * @author Miguel Velasco
 */
#include "Renderer.h"

#include <imGUI/imgui_impl_sdl3.h>
#include <SDL3/SDL_vulkan.h>
#include <imGUI/imgui_impl_vulkan.h>


#include "../Registry/Pipelines.h"
#include "../Registry/Scenes.h"
#include "../Registry/FrameBuffers.h"
#include "../Registry/ComputePipelines.h"

#include "VulkanDevice.h"
#include "Window.h"
#include "Mesh/Mesh.h"
#include "SwapChain.h"
#include "Pipeline.h"
#include "Mesh/FrameBufferObject.h"

#include "../Registry/Meshes.h"



class Menu;
/**
 * @brief Initializes the GUI using ImGui with SDL3 and SDL_Renderer3 backends.
 * @details This function sets up the ImGui context, configures the IO settings, If any of the initialization steps fail, it sets the error flag in the Renderer class to true and prints an error message to the standard error stream.
 */
void Renderer::initGUI(const Window* window)
{
    if (!ImGui::CreateContext()) {
        std::cerr << "Error: No se pudo crear el contexto de ImGui in initGUI()." << std::endl;
        error = true;
        return;
    }   

    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForVulkan(window->window);
 
    ImGui_ImplVulkan_InitInfo info = vulkanDevice->getImGuiInfo(imguiDescriptorPool, instance,swapChain->getImageCount());
    info.PipelineInfoMain.RenderPass   = renderPass;
    info.PipelineInfoMain.Subpass      = 0;
    info.PipelineInfoMain.MSAASamples  = VK_SAMPLE_COUNT_1_BIT;

    if ( !ImGui_ImplVulkan_Init(&info)) {
        std::cerr << "Error in initGUI(): ImGui no se inicializó correctamente in initGUI." << std::endl;
        error = true;
    } 
    #ifdef _DEBUG 
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    std::cout << "ImGui version: " << ImGui::GetVersion() << std::endl;
    std::cout << "Backend Renderer: " << (io.BackendRendererName ? io.BackendRendererName : "null") << std::endl;
    std::cout << "Backend Platform: " << (io.BackendPlatformName ? io.BackendPlatformName : "null") << std::endl;
    std::cout << "Display size: " << io.DisplaySize.x << " x " << io.DisplaySize.y << std::endl;
    
    #endif
}

void Renderer::initVulkan(Window* window)  {

    if (! createVulkanInstance()) {
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
    }

    #ifndef _DEBUG
    std::cout << "(VULKAN) Vulkan surface created successfully.\n";
    #endif

    window->surface = surface;
    vulkanDevice = new VulkanDevice(instance,surface);
        if (vulkanDevice->error) {
        error = true;
        return;
        }
        this->physicalDevice = vulkanDevice->getPhysicalDevice();
        auto* chosenFormat = static_cast<VkSurfaceFormatKHR *>(malloc(sizeof(VkSurfaceFormatKHR)));
        if (!getRenderPassFromSurface(chosenFormat)) {
            error = true;
            return;
        }
        swapChain = new SwapChain(vulkanDevice,window, renderPass, *chosenFormat);
        free(chosenFormat);
        if (swapChain->error){
            error = true;
            return;
        }
        if (!vulkanDevice->createCommandPool()) {
            error = true;
            return;
        }
        if (!vulkanDevice->createCommandBuffers(commandBuffers)) {
            error = true;
            return;
        }
        if (
             !createSyncObjects()
        ) {
            error = true;
            return;
        }

        VkDescriptorPoolSize poolSize1{};
        poolSize1.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize1.descriptorCount = 4;
        VkDescriptorPoolSize poolSize2{};
        poolSize2.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize2.descriptorCount = 5;

        std::vector poolSizes = {poolSize1,poolSize2};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // <-- agregado
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 5;


        vkCreateDescriptorPool(
           vulkanDevice->device,
         &poolInfo,
         nullptr,
         &descriptorPool
        );


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
    const VkDevice device = vulkanDevice->device;
    vkDeviceWaitIdle(device);
    //ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    FrameBuffers::freeFrameBuffers();
   
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);

    // 3. Sincronización (semáforos y fences, uno por frame in-flight normalmente)
    if (!imageAvailableSemaphores.empty() || !renderFinishedSemaphores.empty() || !inFlightFences.empty()) {

    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        

        }
        if (inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
       
        }
      
      }
      for (const auto & renderFinishedSemaphore : renderFinishedSemaphores) {
        if (renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        }
      }
    }


    
    Pipelines::freePipelines();
    ComputePipelines::freePipelines();

    // 7. Render pass
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }
    if (swapChain) {
        delete swapChain;
    }
  
    if (vulkanDevice) {
         delete vulkanDevice;
    }
    // 13. Surface (depende de la instancia, no del device)
    if (surface != VK_NULL_HANDLE){
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    // 14. Instancia (lo último, siempre)
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}


void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al empezar a grabar el command buffer.");
        return;
    }

    // ---- PASADA 1: escena -> FBO ----

    for (const auto& fbo : FrameBuffers::activeFBOs)
    {
        fbo->beginRenderPass(commandBuffer);
        fbo->renderScenes(commandBuffer);
        fbo->endRenderPass(commandBuffer);
    }


    // ---- PASADA 2: FBO -> swapchain (post-proceso) ----
    VkExtent2D swapchainExtent = swapChain->getSwapchainExtent();
    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass  = renderPass;
    renderPassInfo.framebuffer = swapChain->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.extent = swapChain->getSwapchainExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;


    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);



    // Viewport y scissor son dinámicos en el pipeline, hay que setearlos cada frame
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchainExtent.width);
    viewport.height   = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    Pipeline* quadPipeline = FrameBuffers::defaultFrameBuffer->getMultiSamplerPower() > 0 ?Pipelines::getPipeline(POST_PROCESS_PIPELINE_MSAA_ID) : Pipelines::getPipeline(POST_PROCESS_PIPELINE_ID);
    quadPipeline->bind(commandBuffer);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    quadPipeline->getPipelineLayout(), 0, 1, &quadPipeline->descriptorSet, 0, nullptr);
    Meshes::quadMesh->bind(commandBuffer);
    Meshes::quadMesh->draw(commandBuffer);

    //scene->render(commandBuffer); // Renderizar la escena (dibujar los meshes)

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        SDL_Log("(VULKAN) Error: fallo al terminar de grabar el command buffer.");
    }
}

void Renderer::update()
{
    VkDevice device = vulkanDevice->device;

    // Limpiar la pantalla
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // negro
    SDL_RenderClear(renderer);
   // 1. Esperar a que el frame anterior en este slot termine de usarse
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Adquirir la siguiente imagen disponible de la swapchain
    uint32_t imageIndex;
    
    if (!swapChain->acquireNextImage(imageAvailableSemaphores[currentFrame], &imageIndex)){
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
    VkSemaphore signalSemaphores[]    = { renderFinishedSemaphores[imageIndex] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (!vulkanDevice->queueSubmit(&submitInfo, inFlightFences[currentFrame])){
        return;
    }

    // 6. Presentar la imagen ya renderizada
    swapChain->presentImage(imageIndex, signalSemaphores);

    // 7. Avanzar al siguiente frame en vuelo (round-robin entre los N slots)
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


