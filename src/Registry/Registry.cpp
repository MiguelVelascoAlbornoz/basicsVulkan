//
// Created by migue on 28/07/2026.
//
#include "../App/App.h"
#include "Registry.h"

#include "Scenes.h"
#include "../Menu/F3GUI.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Mesh/FrameBufferObject.h"
#include "../Scene/Model.h"
#include "../Renderer/Pipeline.h"
#include "../Renderer/Camera.h"
#include "../Renderer/UniformBuffer.h"
#include "../Renderer/VulkanDevice.h"
#include "../Renderer/Mesh/Mesh.h"
#include "../Menu/EditorMenu.h"

void Registry::registryCallback(const App* app)
{
    Registry::initFramebuffers(app);
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
        {&app->tickDeltaTimeNS,AttribType::FLOAT}
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
    //DEFAULT PIPELINE
    Pipeline::PipelineConfig pipelineConfigDefault;
    pipelineConfigDefault.vertexAttributes = {AttribType::VEC3,AttribType::VEC3};
    pipelineConfigDefault.pushConstantsSize = sizeof(Model::ModelUBO);
    pipelineConfigDefault.bindings = {
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT },
        };
    Pipelines::defaultPipeline = new Pipeline(app->renderer->getVulkanDevice(),FrameBuffers::defaultFrameBuffer->getRenderPass(),pipelineConfigDefault,app->renderer->descriptorPool);

    //LINE PIPELINE
    Pipeline::PipelineConfig linePipelineConfig;
    linePipelineConfig.vertexAttributes = {AttribType::VEC3};
    linePipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    linePipelineConfig.shaderName = "lines";
    linePipelineConfig.bindings = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT },
    };
    linePipelineConfig.pushConstantsSize = sizeof(Uniforms::lineUniform);
    Pipelines::linesPipeline = new Pipeline(app->renderer->getVulkanDevice(),FrameBuffers::defaultFrameBuffer->getRenderPass(),linePipelineConfig,app->renderer->descriptorPool);


    Pipelines::registerPipelines(TEST_PIPELINE_ID,Pipelines::defaultPipeline);
    Pipelines::registerPipelines(LINES_PIPELINE_ID,Pipelines::linesPipeline);

    //POST_PROCESS PIPELINE
    Pipeline::PipelineConfig postProcessPipelineConfig;
    postProcessPipelineConfig.vertexAttributes = {AttribType::VEC2};
    postProcessPipelineConfig.shaderName = "postProcess";
    postProcessPipelineConfig.bindings = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    Pipelines::postProcessPipeline = Pipelines::registerPipelines(POST_PROCESS_PIPELINE_ID, new Pipeline(app->renderer->getVulkanDevice(),app->renderer->renderPass,postProcessPipelineConfig,app->renderer->descriptorPool));
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = FrameBuffers::defaultFrameBuffer->getDepthImageView();
    imageInfo.sampler     = FrameBuffers::defaultFrameBuffer->getDepthSampler();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = Pipelines::postProcessPipeline->descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(app->renderer->getVulkanDevice()->device, 1, &write, 0, nullptr);
}
void Registry::initMeshes(const App* app)
{
    VulkanDevice* device = app->renderer->getVulkanDevice();
    //Solo se pasa la posicion
    std::vector lineVertices = {0.0f,0.0f,0.0f, 1.0f,1.0f,1.0f};
    const std::vector<uint32_t> lineIndices = {0,1};
    Meshes::lineMesh = Meshes::registerMesh(LINE_MESH_ID,new Mesh(device,lineVertices.data(),sizeof(float)*3,2,lineIndices));


    std::vector cubeVertices = {
        //y+                //Normal
        0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,  //1
        0.5f, 0.5f, -0.5f,  0.0f,1.0f,0.0f, //2
        -0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f, //3
        -0.5f, 0.5f, -0.5f,  0.0f,1.0f,0.0f,//4

        //y-               //Normal
       0.5f, -0.5f, 0.5f,  0.0f,-1.0f,0.0f,   //5
       0.5f, -0.5f, -0.5f,  0.0f,-1.0f,0.0f,  //6
       -0.5f, -0.5f, 0.5f,  0.0f,-1.0f,0.0f,  //7
       -0.5f, -0.5f, -0.5f,  0.0f,-1.0f,0.0f, //8

        //x+               //Normal
       0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,   //9
       0.5f, 0.5f, -0.5f, 1.0f,0.0f,0.0f,  //10
       0.5f, -0.5f, 0.5f, 1.0f,0.0f,0.0f,  //11
       0.5f, -0.5f, -0.5f,1.0f,0.0f,0.0f, //12

        //x-               //Normal
       -0.5f, 0.5f, 0.5f,  -1.0f,0.0f,0.0f,   //13
       -0.5f, 0.5f, -0.5f, -1.0f,0.0f,0.0f,  //14
       -0.5f, -0.5f, 0.5f, -1.0f,0.0f,0.0f,  //15
       -0.5f, -0.5f, -0.5f,-1.0f,0.0f,0.0f, //16

        //z+                //Normal
       0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,   //17
       -0.5f, 0.5f, 0.5f, 0.0f,0.0f,1.0f,  //18
       0.5f, -0.5f,0.5f, 0.0f,0.0f,1.0f,  //19
       -0.5f, -0.5f,0.5f, 0.0f,0.0f,1.0f, //20

        //z-                //Normal
       0.5f, 0.5f, -0.5f,  0.0f,0.0f,-1.0f,   //21
       -0.5f, 0.5f,-0.5f, 0.0f,0.0f,-1.0f,  //22
       0.5f, -0.5f,-0.5f, 0.0f,0.0f,-1.0f,  //23
       -0.5f, -0.5f,-0.5f, 0.0f,0.0f,-1.0f, //24

    };
    std::vector<uint32_t> cubeIndices = {
        // Y+
        0, 2, 1,
        1, 2, 3,

        // Y-
        4, 5, 6,
        5, 7, 6,

        // X+
        8, 10, 9,
        9, 10, 11,

        // X-
        12, 13, 14,
        13, 15, 14,

        // Z+
        16, 18, 17,
        17, 18, 19,

        // Z-
        20, 21, 22,
        21, 23, 22
    };
    Meshes::cubeMesh = Meshes::registerMesh(CUBE_MESH_ID, new Mesh(device,cubeVertices.data(),sizeof(float)*6,24,cubeIndices));

    std::vector quadVertices = {
        1.0f,1.0f,
        1.0f,-1.0f,
        -1.0f,-1.0f,
        -1.0f,1.0f,
    };
    std::vector<uint32_t> quadIndices = {
        0, 1, 2,
        2, 3, 0
    };
    Meshes::quadMesh = Meshes::registerMesh(QUAD_MESH_ID, new Mesh(device,quadVertices.data(),sizeof(float)*8,4,quadIndices));
}

void Registry::initMenus(const App* app)
{
    //Registers
    Menus::editorMenu = dynamic_cast<EditorMenu*>(Menus::registerMenu(EDITOR_MENU_ID,new EditorMenu(app->player,[](){Uniforms::onPlayerRenderUpdate();})));
    Menus::F3Menu = dynamic_cast<F3GUI*>(Menus::registerMenu(F3_MENU_ID,new F3GUI(app)));
}

void Registry::initFramebuffers(const App* app)
{
    FrameBuffers::defaultFrameBuffer = FrameBuffers::registerFrameBuffer(DEFAULT_FRAME_BUFFER_ID,new  FrameBufferObject(app->renderer->getVulkanDevice(),800,600,VK_FORMAT_R8G8B8A8_UNORM,true,true));
    FrameBuffers::defaultFrameBuffer->addScene(Scenes::renderTest);
}
