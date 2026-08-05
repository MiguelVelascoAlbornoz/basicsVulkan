//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_PIPELINES_H
#define BASICSVULKAN_PIPELINES_H


#include "../App/Utilitys.h"

#define TEST_PIPELINE_ID "test_pipeline"
#define LINES_PIPELINE_ID "lines_pipeline"
#define POST_PROCESS_PIPELINE_ID "post_process_pipeline"

class Pipeline;

class Pipelines
{
    public:
    static Pipeline* defaultPipeline;
    static Pipeline* linesPipeline;
    static Pipeline* postProcessPipeline;

    static std::unordered_map<std::string, Pipeline*> pipelines; /**< @brief Map to store menu rendering functions. */

    static Pipeline* registerPipelines(const std::string &pipelineID, Pipeline* pipeline);
    static void freePipelines();

    static Pipeline* getPipeline(const std::string& pipelineID) {
        return pipelines[pipelineID];
    }
    static void callForEveryOne(void call(Pipeline*))
    {
        for (const auto& pipeline : pipelines)
        {
            call(pipeline.second);
        }
    }

};


#endif //BASICSVULKAN_PIPELINES_H
