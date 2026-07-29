//
// Created by migue on 29/07/2026.
//

#ifndef BASICSVULKAN_SCENES_H
#define BASICSVULKAN_SCENES_H
#include "vulkan/vulkan_core.h"
#include <vector>


class Scenes
{
    using SceneFunction = void(*)(VkCommandBuffer);
    public:
    static void renderAxis( VkCommandBuffer commandBuffer);
    static void turnOnScene(SceneFunction onScene);
    static void turnOffScene(SceneFunction offScene);
    static std::vector<SceneFunction> activeScenes;
};


#endif //BASICSVULKAN_SCENES_H
