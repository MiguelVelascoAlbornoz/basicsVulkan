//
// Created by migue on 10/08/2026.
//

#include "ComputePipelines.h"
#include "../Renderer/ComputePipeline.h"
#include "../App/Logger.h"

std::unordered_map<std::string, ComputePipeline*> ComputePipelines::computePipelines;

ComputePipeline* ComputePipelines::registerPipelines(const std::string &pipelineID, ComputePipeline* pipeline){
    if (pipeline->error) {

        stdout_logger.log("Pipeline with ID \"", pipelineID, "\" unsuccesfully initialized.");
    }
    return registerObject(pipelineID,pipeline,computePipelines);
}

void ComputePipelines::freePipelines()
{

    for (const auto& pipeline : computePipelines) {
        delete pipeline.second;
    }

}