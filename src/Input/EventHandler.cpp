
#include <imGUI/imgui_impl_sdl3.h>
#include "../App/App.h"
#include "../Renderer/Window.h"
#include "../Registry/Scenes.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/UniformBuffer.h"
#include "../renderer/VulkanDevice.h"
#include "../renderer/Mesh/FrameBufferObject.h"
#include "../renderer/Pipeline.h"

void App::manageEvents() {
    SDL_Event event;
    movedMouse = false;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
           
          runnig = false;
          continue;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_ESCAPE:

                    runnig = false;
                    break;
                case SDLK_F11:
                    this->window->toggleFullscreen();
                    break;
                case SDLK_F3:

                    this->F3Mode = !this->F3Mode;
                    if (F3Mode)
                    {
                        FrameBuffers::defaultFrameBuffer->addScene(Scenes::renderAxis);
                        Menus::openMenu(F3_MENU_ID);
                    } else
                    {
                        FrameBuffers::defaultFrameBuffer->removeScene(Scenes::renderAxis);
                        Menus::closeMenu(F3_MENU_ID);
                    }
                    break;
                case SDLK_T:
                    if (editorMode) {
                        SDL_SetWindowRelativeMouseMode(window->window, editorMode);
                        Menus::closeMenu(EDITOR_MENU_ID);
                    } else {
                        SDL_SetWindowRelativeMouseMode(window->window, editorMode);
                        Menus::openMenu(EDITOR_MENU_ID);
                    }
                    this->editorMode = !this->editorMode;
                    break;
                default:
                    break;
            }
        } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            std::cout << "Window close requested." << std::endl;
            runnig = false;
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            if (!renderer->onWindowResized(window)) {
                std::cerr << "Failed to handle window resize event." << std::endl;
                runnig = false;
            }
        }  if (event.type == SDL_EVENT_MOUSE_MOTION) {
            mouseMotion = event.motion;
            movedMouse = true;


        }
    }

}
void App::managePlayerMovement() {
    const bool* keys = SDL_GetKeyboardState(nullptr);


    mat3 cameraWorldMatrix = player->camera->getWorldMatrix();


    vec3 delta = vec3();
    if (keys[SDL_SCANCODE_SPACE])
    {
        delta = delta + cameraWorldMatrix[0];

    }
    if (keys[SDL_SCANCODE_W]) {
        delta = delta +cameraWorldMatrix[1];

    }
    if (keys[SDL_SCANCODE_S]) {
        delta =delta - cameraWorldMatrix[1];

    }
    if (keys[SDL_SCANCODE_A]) {
        delta =delta + cameraWorldMatrix[2];

    }
    if (keys[SDL_SCANCODE_D]) {
        delta =delta - cameraWorldMatrix[2];

    }
    if (keys[SDL_SCANCODE_LSHIFT]) {
        delta =delta - cameraWorldMatrix[0];

    }
    if (delta.x != 0 || delta.y != 0 || delta.z != 0) {
        delta = normalize(delta);
        delta = delta*player->speed;
        if (keys[SDL_SCANCODE_LCTRL]) {
            delta = delta*player->speedMultiplier;
        }
        player->move(delta);
        Uniforms::onPlayerRenderUpdate();
    }
}
void App::onFBOResolutionChange() const {
    vkDeviceWaitIdle(renderer->getVulkanDevice()->device);
    FrameBuffers::defaultFrameBuffer->changeResolution(window->getWidth()*player->getPlayerCameraSettings()->fboResolutionMultiplier, window->getHeight()*player->getPlayerCameraSettings()->fboResolutionMultiplier);
    //Pipelines::postProcessPipeline->updateDescriptorSet();
}
void App::manageCameraRotation(SDL_MouseMotionEvent event) {
        if (!movedMouse) return;

        vec4 cameraRotation =  player->camera->getCameraRotation();
        float realSensibility =  player->rotationSensitivity;
        vec3 newFacing(cameraRotation.x + event.xrel* realSensibility, cameraRotation.y - event.yrel * realSensibility, cameraRotation.z);
         player->setRotation(newFacing.x,newFacing.y,newFacing.z,cameraRotation.w);
        Uniforms::cameraUniform->addIndexToQueue(Uniforms::CameraUBO::VIEW_PROJECTION_MATRIX);
}
