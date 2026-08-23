//
// Created by migue on 28/07/2026.
//
#include "../App/App.h"
#include "Registry.h"

#include "ComputePipelines.h"
#include "Scenes.h"
#include "../Menu/F3GUI.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Mesh/FrameBufferObject.h"
#include "../Scene/Model.h"
#include "../Renderer/Pipeline.h"
#include "../Renderer/Camera.h"
#include "../Renderer/UniformBuffer.h"
#include "Images.h"
#include "../Renderer/Image.h"
#include "../Renderer/Mesh/Mesh.h"
#include "../Menu/EditorMenu.h"
#include "../renderer/Window.h"
#include "../Renderer/ComputePipeline.h"


void Registry::registryCallback(const App* app)
{   Registry::initImages(app);
    Registry::initFramebuffers(app);
    Registry::initUniforms(app);
    Registry::initPipelines(app);
    Registry::initComputePipelines(app);
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
        {&app->tickDeltaTimeNS,AttribType::FLOAT},
    {player->camera->getPosition(),AttribType::VEC3},
        {player->camera->getViewDirection(),AttribType::VEC3},
    };
    Uniforms::cameraUniform = Uniforms::registerUniform(CAMERA_UNIFORM_ID, new UniformBuffer(device,inputsMap));


}

void Registry::initPipelines(const App* app)
{
    //Register pipelines
    //DEFAULT PIPELINE
    PipelineConfig pipelineConfigDefault;
    pipelineConfigDefault.vertexAttributes = {AttribType::VEC3,AttribType::VEC3};
    pipelineConfigDefault.pushConstantsSize = sizeof(Model::ModelUBO);
    pipelineConfigDefault.uniformObjects = {
                {Uniforms::cameraUniform, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT },
    };
    pipelineConfigDefault.images = {
        {
            .image = Images::missingImage->getView(),
            .sampler = Images::missingImage->getSampler(),
            .layout = Images::missingImage->getCurrentLayout(),
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
        }
    };
    pipelineConfigDefault.multisamplerSamples = app->player->getPlayerCameraSettings()->MSAAsamples;
    pipelineConfigDefault.sampleShadding = app->player->getPlayerCameraSettings()->sampleShadding;
    Pipelines::defaultPipeline = new Pipeline(app->renderer->getVulkanDevice(),FrameBuffers::defaultFrameBuffer->getRenderPass(),pipelineConfigDefault,app->renderer->descriptorPool);

    //LINE PIPELINE
    PipelineConfig linePipelineConfig;
    linePipelineConfig.vertexAttributes = {AttribType::VEC3};
    linePipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    linePipelineConfig.shaderName = "lines";
    linePipelineConfig.uniformObjects = {
            {Uniforms::cameraUniform, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT },
    };
    linePipelineConfig.multisamplerSamples = app->player->getPlayerCameraSettings()->MSAAsamples;
    linePipelineConfig.sampleShadding = app->player->getPlayerCameraSettings()->sampleShadding;

    linePipelineConfig.pushConstantsSize = sizeof(Uniforms::lineUniform);
    Pipelines::linesPipeline = new Pipeline(app->renderer->getVulkanDevice(),FrameBuffers::defaultFrameBuffer->getRenderPass(),linePipelineConfig,app->renderer->descriptorPool);

    //POST_PROCESS PIPELINE
    PipelineConfig postProcessPipelineConfig;
    postProcessPipelineConfig.vertexAttributes = {AttribType::VEC2};
    postProcessPipelineConfig.shaderName = "postProcess";
    Pipelines::getPostProcessPipelineConfig(&postProcessPipelineConfig,app->player->getPlayerCameraSettings()->MSAAsamples > 0,Pipelines::NO_MSAA);

    //POST_PROCESS_PIPELINE_MSAA
    PipelineConfig postProcessPipelineConfigMSAA =postProcessPipelineConfig ;
    postProcessPipelineConfigMSAA.shaderName = "postProcessMSAA";
    Pipelines::getPostProcessPipelineConfig(&postProcessPipelineConfigMSAA,app->player->getPlayerCameraSettings()->MSAAsamples > 0,Pipelines::MSAA);



    Pipelines::registerPipelines(TEST_PIPELINE_ID,Pipelines::defaultPipeline);
    Pipelines::registerPipelines(LINES_PIPELINE_ID,Pipelines::linesPipeline);
    Pipelines::postProcessPipeline = Pipelines::registerPipelines(POST_PROCESS_PIPELINE_ID, new Pipeline(app->renderer->getVulkanDevice(),app->renderer->renderPass,postProcessPipelineConfig,app->renderer->descriptorPool));
    Pipelines::postProcessPipelineMSAA = Pipelines::registerPipelines(POST_PROCESS_PIPELINE_MSAA_ID, new Pipeline(app->renderer->getVulkanDevice(),app->renderer->renderPass,postProcessPipelineConfigMSAA,app->renderer->descriptorPool));


    FrameBuffers::defaultFrameBuffer->registerDependentPipeline(Pipelines::defaultPipeline);
    FrameBuffers::defaultFrameBuffer->registerDependentPipeline(Pipelines::linesPipeline);
}
void Registry::initMeshes(const App* app)
{
    VulkanDevice* device = app->renderer->getVulkanDevice();
    //Solo se pasa la posicion
    std::vector lineVertices = {0.0f,0.0f,0.0f, 1.0f,1.0f,1.0f};
    const std::vector<uint32_t> lineIndices = {0,1};
    Meshes::lineMesh = Meshes::registerMesh(LINE_MESH_ID,new Mesh(device,lineVertices.data(),sizeof(float)*3,2,lineIndices));

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

    std::vector<float> planeMeshVertices;
    std::vector<uint32_t> planeMeshIndices;
    int resolution = 1000;
    Meshes::generatePlaneMesh(planeMeshVertices,planeMeshIndices,resolution);
    Meshes::registerMesh("plane_mesh", new Mesh(device,planeMeshVertices.data(),sizeof(float)*6,resolution*resolution,planeMeshIndices));
}

void Registry::initMenus(const App* app)
{
    //Registers
    Menus::editorMenu = dynamic_cast<EditorMenu*>(Menus::registerMenu(EDITOR_MENU_ID,new EditorMenu(app)));
    Menus::F3Menu = dynamic_cast<F3GUI*>(Menus::registerMenu(F3_MENU_ID,new F3GUI(app)));
}

VkFormat getFormatByBits(Player::PlayerCameraSettings::BitsPerChannel bitPerChannel)
{
    switch (bitPerChannel)
    {

    case Player::PlayerCameraSettings::BitsPerChannel::BITS_16:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Player::PlayerCameraSettings::BitsPerChannel::BITS_32:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    default:
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

void Registry::initFramebuffers(const App* app)
{
    FrameBuffers::defaultFrameBuffer = FrameBuffers::registerFrameBuffer(
        DEFAULT_FRAME_BUFFER_ID,
        new FrameBufferObject(
            app->renderer->getVulkanDevice(),
            app->window->getWidth()*app->player->getPlayerCameraSettings()->fboResolutionMultiplier,
            app->window->getHeight()*app->player->getPlayerCameraSettings()->fboResolutionMultiplier,
            getFormatByBits(app->player->getPlayerCameraSettings()->bitsPerChannel),
            true,
            true,app->player->getPlayerCameraSettings()->MSAAsamples
        )
    );
    FrameBuffers::defaultFrameBuffer->addScene(Scenes::renderTest);

}

void Registry::initImages(const App* app)
{

    Images::missingImage = Images::registerImages(MISSING_IMAGE_ID,
        Image::loadFromFile(app->renderer->getVulkanDevice(),"assets/Textures/missing_texture.png",VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
}
void Registry::initComputePipelines(const App* app)
{
    app->player->getPosition();//EVITAR WARNINGS
}
