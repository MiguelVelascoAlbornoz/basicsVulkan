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
        {&app->tickDeltaTimeNS,AttribType::FLOAT}
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
    Image* imageToPipeline = Images::images[GRASS_IMAGE_ID];
    pipelineConfigDefault.images = {
        {
            .image = imageToPipeline->getView(),
            .sampler = imageToPipeline->getSampler(),
            .layout = imageToPipeline->getCurrentLayout(),
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
        },
            {
            .image = Images::images[IFFT_OUT_IMAGE_ID]->getView(),
            .sampler = Images::images[IFFT_OUT_IMAGE_ID]->getSampler(),
            .layout = Images::images[IFFT_OUT_IMAGE_ID]->getCurrentLayout(),
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

void Registry::initFramebuffers(const App* app)
{
    FrameBuffers::defaultFrameBuffer = FrameBuffers::registerFrameBuffer(
        DEFAULT_FRAME_BUFFER_ID,
        new FrameBufferObject(
            app->renderer->getVulkanDevice(),
            app->window->getWidth()*app->player->getPlayerCameraSettings()->fboResolutionMultiplier,
            app->window->getHeight()*app->player->getPlayerCameraSettings()->fboResolutionMultiplier,
            VK_FORMAT_R8G8B8A8_UNORM,
            true,
            true,app->player->getPlayerCameraSettings()->MSAAsamples
        )
    );
    FrameBuffers::defaultFrameBuffer->addScene(Scenes::renderTest);

}

void Registry::initImages(const App* app)
{

    Images::missingImage = Images::registerImages(MISSING_IMAGE_ID,Image::loadFromFile(app->renderer->getVulkanDevice(),"assets/Textures/missing_texture.png",VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
    Images::ifftInImage = Images::registerImages(IFFT_IN_IMAGE_ID,new Image(app->renderer->getVulkanDevice(),app->FFT_N,app->FFT_N,VK_FORMAT_R32G32B32A32_SFLOAT,VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_COLOR_BIT,{.magFilter  = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR, .adressMode =  VK_SAMPLER_ADDRESS_MODE_REPEAT},0));
    Images::registerImages(IFFT_OUT_IMAGE_ID,new Image(app->renderer->getVulkanDevice(),512,512,VK_FORMAT_R32G32B32A32_SFLOAT,VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,{.magFilter  = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR, .adressMode =  VK_SAMPLER_ADDRESS_MODE_REPEAT},0));
    Images::registerImages(IFFT_DERIVATES_TEMP_IMAGE_ID,new Image(app->renderer->getVulkanDevice(),512,512,VK_FORMAT_R32G32B32A32_SFLOAT,VK_IMAGE_USAGE_STORAGE_BIT,VK_IMAGE_ASPECT_COLOR_BIT,{.magFilter  = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR, .adressMode =  VK_SAMPLER_ADDRESS_MODE_REPEAT},0));

    Images::registerImages(GRASS_IMAGE_ID,Image::loadFromFile(app->renderer->getVulkanDevice(),"assets/Textures/grass.png",VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));

    VkCommandBuffer cmd = app->renderer->getVulkanDevice()->beginSingleTimeCommands();
    Images::images[IFFT_OUT_IMAGE_ID]->transitionLayout(cmd,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    app->renderer->getVulkanDevice()->endSingleTimeCommands(cmd);
}

void Registry::initComputePipelines(const App* app)
{
    ImageBinding ifftInBinding =   {
        .image = Images::ifftInImage->getView(),
        .sampler = VK_NULL_HANDLE,
        .layout =VK_IMAGE_LAYOUT_GENERAL,
        .type =VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
    };

    ComputePipelineConfig ifftConfig;
    ifftConfig.images = {
        ifftInBinding,
{
        .image = Images::images[IFFT_OUT_IMAGE_ID]->getView(),
        .sampler = VK_NULL_HANDLE,
        .layout =VK_IMAGE_LAYOUT_GENERAL,
        .type =VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
},{
    .image = Images::images[IFFT_DERIVATES_TEMP_IMAGE_ID]->getView(),
            .sampler = VK_NULL_HANDLE,
            .layout =VK_IMAGE_LAYOUT_GENERAL,
            .type =VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT}
    };
    ifftConfig.shaderMacros = {"FFT_N","512"};
    ifftConfig.shaderName = "ifft";
    ifftConfig.pushConstantsSize = sizeof(ComputePipelines::IfftComputePushConstants);
    ComputePipelines::registerPipelines(IFFT_COMPUTE_PIPELINE_ID,new ComputePipeline(app->renderer->getVulkanDevice(),app->renderer->descriptorPool,ifftConfig));


    ComputePipelineConfig setWavesPipelineConfig;
    setWavesPipelineConfig.shaderName = "setWaves";
    setWavesPipelineConfig.images = {
        ifftInBinding
    };

    ComputePipelines::registerPipelines(IFFT_SET_WAVES_COMPUTE_PIPELINE_ID,new ComputePipeline(
        app->renderer->getVulkanDevice(),app->renderer->descriptorPool,setWavesPipelineConfig));
}
