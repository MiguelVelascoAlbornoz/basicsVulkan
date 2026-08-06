//
// Created by migue on 28/07/2026.
//

#include "Pipelines.h"
#include "../Renderer/Pipeline.h"
std::unordered_map<std::string, Pipeline*> Pipelines::pipelines;

Pipeline* Pipelines::defaultPipeline;
Pipeline* Pipelines::linesPipeline;
Pipeline* Pipelines::postProcessPipeline;
Pipeline* Pipelines::postProcessPipelineMSAA;

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
