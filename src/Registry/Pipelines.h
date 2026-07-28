//
// Created by migue on 28/07/2026.
//

#ifndef BASICSVULKAN_PIPELINES_H
#define BASICSVULKAN_PIPELINES_H

#include "../Renderer/Renderer.h"
#include "../App/Utilitys.h"

#define TEST_PIPELINE_ID "test_pipeline"
#define LINES_PIPELINE_ID "lines_pipeline"
class Pipelines
{
    public:
    static Pipeline* defaultPipeline;
    static Pipeline* linesPipeline;
    static std::unordered_map<std::string, Pipeline*> pipelines; /**< @brief Map to store menu rendering functions. */

    static Pipeline* registerPipelines(const std::string &pipelineID, Pipeline* pipeline) {
        if (pipeline->error) {

            std::cout << "Pipeline with ID \"" << pipelineID << "\" unsuccesfully initialized."<< std::endl;
        }
        return registerObject(pipelineID,pipeline,pipelines);
    }
    static void freePipelines() {
        for (const auto& pipeline : pipelines) {
            delete pipeline.second;
        }
    }

    static Pipeline* getPipeline(const std::string& pipelineID) {
        return pipelines[pipelineID];
    }

};


#endif //BASICSVULKAN_PIPELINES_H
