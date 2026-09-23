//
// Created by migue on 28/07/2026.
//

#include "Pipelines.h"
#include "../Renderer/Pipeline.h"
#include "FrameBuffers.h"
#include "../Renderer/Mesh/FrameBufferObject.h"

std::unordered_map<std::string, Pipeline*> Pipelines::pipelines;

Pipeline* Pipelines::defaultPipeline;
Pipeline* Pipelines::linesPipeline;
Pipeline* Pipelines::postProcessPipeline;
Pipeline* Pipelines::postProcessPipelineMSAA;

void Pipelines::getPostProcessPipelineConfig(PipelineConfig* config, bool MSAA, PostProcessType type)
{
    if (type == PostProcessType::MSAA)
    {
        MSAA = !MSAA;
    }
    config->images = {{
        MSAA?  VK_NULL_HANDLE : FrameBuffers::defaultFrameBuffer->getColorImageView() ,
        FrameBuffers::defaultFrameBuffer->getColorSampler(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,//VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            MSAA ?  VK_NULL_HANDLE : FrameBuffers::defaultFrameBuffer->getDepthImageView() ,
            FrameBuffers::defaultFrameBuffer->getColorSampler(),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT
            }
    };
}

void Pipelines::updatePostProcessPipelinesDescriptors(bool MSAA)
{
    PipelineConfig configTemp;
    Pipelines::getPostProcessPipelineConfig(&configTemp,MSAA,Pipelines::NO_MSAA);
    Pipelines::postProcessPipeline->updateDescriptorSet({},configTemp.images);
    Pipelines::getPostProcessPipelineConfig(&configTemp,MSAA,Pipelines::MSAA);
    Pipelines::postProcessPipelineMSAA->updateDescriptorSet({},configTemp.images);

}


Pipeline* Pipelines::registerPipelines(const std::string &pipelineID, Pipeline* pipeline){
    if (pipeline->error) {

        std::cout << "Pipeline with ID \"" << pipelineID << "\" unsuccesfully initialized."<< std::endl;
    }
    return registerObject(pipelineID,pipeline,pipelines);
}

void Pipelines::freePipelines()
{

        for (const auto& pipeline : pipelines) {
            delete pipeline.second;
        }

}
