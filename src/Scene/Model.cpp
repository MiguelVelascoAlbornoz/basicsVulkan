//
// Created by migue on 30/07/2026.
//

#include "Model.h"

#include "glm/ext/matrix_transform.hpp"
#include <Vulkan/vulkan_core.h>
#include "../Renderer/Pipeline.h"
#include "../Renderer/Mesh/Mesh.h"

void Model::setRotation(vec3 angles)
{
    this->angles = angles;

    model.rotationMatrix =  glm::rotate(glm::mat4(1.0f), angles.x, glm::vec3(1.0f, 0.0f, 0.0f))* glm::rotate(glm::mat4(1.0f), angles.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), angles.z, glm::vec3(0.0f, 0.0f, 1.0f))  ;
    calculateModelMatrix();
}
void Model::setTranslation(vec3 position)
{
    this->position = position;
    translationMatrix = glm::translate(glm::mat4(1.0f), position);
    calculateModelMatrix();

}
void Model::draw(VkCommandBuffer commandBuffer, Pipeline* pipeline)
{
    vkCmdPushConstants(
        commandBuffer,
        pipeline->getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(ModelUBO),
        &model
    );

    mesh->bind(commandBuffer);
    mesh->draw(commandBuffer);
}

void Model::setModelMatrix(mat4 modelMatrix)
{
    model.modelMatrix = modelMatrix;
}

void Model::setScale(vec3 scale)
{
    model.scale = scale;
}

void Model::calculateModelMatrix()
{
    model.modelMatrix = translationMatrix * model.rotationMatrix;
}
