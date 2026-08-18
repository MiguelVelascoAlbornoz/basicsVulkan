//
// Created by migue on 10/08/2026.
//

#include "ComputePipelines.h"
#include "../Renderer/ComputePipeline.h"

std::unordered_map<std::string, ComputePipeline*> ComputePipelines::computePipelines;

ComputePipeline* ComputePipelines::registerPipelines(const std::string &pipelineID, ComputePipeline* pipeline){
    if (pipeline->error) {

        std::cout << "Pipeline with ID \"" << pipelineID << "\" unsuccesfully initialized."<< std::endl;
    }
    return registerObject(pipelineID,pipeline,computePipelines);
}

void ComputePipelines::freePipelines()
{

    for (const auto& pipeline : computePipelines) {
        delete pipeline.second;
    }

}