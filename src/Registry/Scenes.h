//
// Created by migue on 29/07/2026.
//

#ifndef BASICSVULKAN_SCENES_H
#define BASICSVULKAN_SCENES_H
#include "vulkan/vulkan_core.h"
#include <vector>


class Scenes
{

    public:
    using SceneFunction = void(*)(VkCommandBuffer);
    static void renderAxis( VkCommandBuffer commandBuffer);
    static void renderTest( VkCommandBuffer commandBuffer);


};


#endif //BASICSVULKAN_SCENES_H
