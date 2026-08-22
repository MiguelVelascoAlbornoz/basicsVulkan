//
// Created by migue on 29/07/2026.
//

#include "Scenes.h"
#include "../Renderer/Pipeline.h"
#include "Meshes.h"
#include "Pipelines.h"
#include "Uniforms.h"
#include "../Renderer/ComputePipeline.h"
#include "../Scene/Model.h"
#include "../Renderer/Image.h"

#include "../Renderer/Mesh/Mesh.h"

void Scenes::renderAxis( const VkCommandBuffer commandBuffer)
{

    Pipeline* linesPipeline = Pipelines::getPipeline(LINES_PIPELINE_ID);
    linesPipeline->bind(commandBuffer); // bind the graphics pipeline (shaders + fixed-function state);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,linesPipeline->getPipelineLayout(),0,1,&linesPipeline->descriptorSet,0,nullptr);

    Meshes::meshes[LINE_MESH_ID]->bind(commandBuffer);
    Uniforms::lineUniform.color = glm::vec3(1.0f, .0f, .0f);
    Uniforms::lineUniform.direction = glm::vec3(1000000000.0f, .0f, .0f);
    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);

    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);

    Uniforms::lineUniform.color = glm::vec3(.0f, 1.f, .0f);
    Uniforms::lineUniform.direction = glm::vec3(.0f, 1000000000.f, .0f);
    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);


    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);

    Uniforms::lineUniform.direction = glm::vec3(.0f, .0f, 1000000000.0f);
    Uniforms::lineUniform.color = glm::vec3(0.0f, 0.0f, 1.0f);

    vkCmdPushConstants(commandBuffer, linesPipeline->getPipelineLayout(),VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Uniforms::lineUniform), &Uniforms::lineUniform);


    Meshes::meshes[LINE_MESH_ID]->draw(commandBuffer);
}

void Scenes::renderTest(const VkCommandBuffer commandBuffer) {


    Model cubeModel;
    cubeModel.mesh = Meshes::cubeMesh;
    cubeModel.setTranslation(vec3(0.0f,0.0f,0.0f));
    cubeModel.setRotation(vec3(324,424,0.0f));
    cubeModel.setScale(vec3(1.0f,1.0f,1.0f));

    Pipeline* testPipeline = Pipelines::getPipeline(TEST_PIPELINE_ID);
    testPipeline->bind(commandBuffer); // bind the graphics pipeline (shaders + fixed-function state);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,testPipeline->getPipelineLayout(),0,1,&testPipeline->descriptorSet,0,nullptr);


    cubeModel.draw(commandBuffer,testPipeline);

}


