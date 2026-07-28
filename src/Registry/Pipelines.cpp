//
// Created by migue on 28/07/2026.
//

#include "Pipelines.h"
std::unordered_map<std::string, Pipeline*> Pipelines::pipelines;

Pipeline* Pipelines::defaultPipeline;
Pipeline* Pipelines::linesPipeline;