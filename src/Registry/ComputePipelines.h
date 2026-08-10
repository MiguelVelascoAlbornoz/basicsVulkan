//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_COMPUTE_PIPELINES_H
#define BASICSVULKAN_COMPUTE_PIPELINES_H


#include "../App/Utilitys.h"

#define IFFT_COMPUTE_PIPELINE_ID "ifft_compute_pipeline"

class ComputePipeline;

class ComputePipelines
{
public:
    static std::unordered_map<std::string, ComputePipeline*> computePipelines; /**< @brief Map to store menu rendering functions. */


    static ComputePipeline* registerPipelines(const std::string &pipelineID, ComputePipeline* computePipeline);
    static void freePipelines();

    static ComputePipeline* getPipeline(const std::string& computePipelineID) {
        return computePipelines[computePipelineID];
    }


};


#endif //BASICSVULKAN_PIPELINES_H
