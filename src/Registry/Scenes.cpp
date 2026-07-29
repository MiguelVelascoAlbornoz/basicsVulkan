//
// Created by migue on 29/07/2026.
//

#include "Scenes.h"
#include "../Renderer/Pipeline.h"
#include "Meshes.h"
#include "Pipelines.h"
#include "Uniforms.h"

void Scenes::renderAxis( const VkCommandBuffer commandBuffer)
{

    Pipeline* linesPipeline = Pipelines::getPipeline(LINES_PIPELINE_ID);
    linesPipeline->bind(commandBuffer); // bind the graphics pipeline (shaders + fixed-function state);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,linesPipeline->getPipelineLayout(),0,1,&linesPipeline->descriptorSet,0,nullptr);

    Meshes::meshes[LINE_MESH_ID]->bind(commandBuffer);
    Uniforms::lineUniform.color = glm::vec3(1.0f, .0f, .0f);
    Uniforms::lineUniform.direction = glm::vec3(1.0f, .0f, .0f);
    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);

    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);

    Uniforms::lineUniform.color = glm::vec3(.0f, 1.f, .0f);
    Uniforms::lineUniform.direction = glm::vec3(.0f, 1.f, .0f);
    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);


    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);

    Uniforms::lineUniform.color = glm::vec3(.0f, .0f, 1.0f);

    Uniforms::lineUniform.direction = glm::vec3(.0f, .0f, 1.0f);
    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);


    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);
}
