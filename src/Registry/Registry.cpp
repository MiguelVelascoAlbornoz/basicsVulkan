//
// Created by migue on 28/07/2026.
//

#include "Registry.h"
#include "../Menu/F3GUI.h"
#include "../Renderer/Pipeline.h"
void Registry::registryCallback(const App* app)
{
    Registry::initPipelines(app);
    Registry::initUniforms(app);
    Registry::initMeshes(app);
    Registry::initMenus(app);
};

void Registry::initUniforms(const App* app)
{
    const Renderer* renderer = app->renderer;
    const Player* player = app->player;


    VulkanDevice* device = renderer->getVulkanDevice();

    std::vector<std::pair<const void*,AttribType::INPUT_TYPES>> inputsMap = {
        {player->camera->getViewProjectionMatrix(),AttribType::MAT4},
        {&app->currentTimeInSeconds,AttribType::FLOAT}
    };
    Uniforms::cameraUniform = Uniforms::registerUniform(CAMERA_UNIFORM_ID, new UniformBuffer(device,inputsMap));

    std::vector<VkDescriptorBufferInfo> bufferInfosTest(1);
    std::vector<VkWriteDescriptorSet> writesTest;
    writesTest.push_back(Uniforms::cameraUniform->getWriteDescriptor(
        Pipelines::defaultPipeline->descriptorSet, 0, bufferInfosTest[0]));
    vkUpdateDescriptorSets(device->device, 1, writesTest.data(), 0, nullptr);

    std::vector<VkDescriptorBufferInfo> bufferInfos(1);
    std::vector<VkWriteDescriptorSet> writes;

    writes.push_back(Uniforms::cameraUniform->getWriteDescriptor(
        Pipelines::linesPipeline->descriptorSet, 0, bufferInfos[0]));

    vkUpdateDescriptorSets(device->device,static_cast<uint32_t>(writes.size()), writes.data(),0, nullptr);


}

void Registry::initPipelines(const App* app)
{
    //Register pipelines
    Pipeline::PipelineConfig pipelineConfigDefault;
    pipelineConfigDefault.vertexAttributes = {AttribType::VEC3,AttribType::VEC3};
    Pipelines::defaultPipeline = new Pipeline(app->renderer->getVulkanDevice(),app->renderer->renderPass,pipelineConfigDefault,app->renderer->descriptorPool);

    Pipeline::PipelineConfig linePipelineConfig;
    linePipelineConfig.vertexAttributes = {AttribType::VEC3};
    linePipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    linePipelineConfig.shaderName = "lines";
    linePipelineConfig.bindingsCount = 1;
    linePipelineConfig.pushConstantsSize = sizeof(Uniforms::lineUniform);
    Pipelines::linesPipeline = new Pipeline(app->renderer->getVulkanDevice(),app->renderer->renderPass,linePipelineConfig,app->renderer->descriptorPool);


    Pipelines::registerPipelines(TEST_PIPELINE_ID,Pipelines::defaultPipeline);
    Pipelines::registerPipelines(LINES_PIPELINE_ID,Pipelines::linesPipeline);
}
void Registry::initMeshes(const App* app)
{
    VulkanDevice* device = app->renderer->getVulkanDevice();
    //Solo se pasa la posicion
    std::vector lineVertices = {0.0f,0.0f,0.0f, 1.0f,1.0f,1.0f};
    const std::vector<uint32_t> lineIndices = {0,1};
    Meshes::lineMesh = Meshes::registerMesh(LINE_MESH_ID,new Mesh(device,lineVertices.data(),sizeof(float)*3,2,lineIndices));
}

void Registry::initMenus(const App* app)
{

    //Registers
    Menus::editorMenu = dynamic_cast<EditorMenu*>(Menus::registerMenu(EDITOR_MENU_ID,new EditorMenu(app->player,[](){Uniforms::onPlayerRenderUpdate();})));
    Menus::F3Menu = dynamic_cast<F3GUI*>(Menus::registerMenu(F3_MENU_ID,new F3GUI(app->player,&app->deltaTime)));

}
