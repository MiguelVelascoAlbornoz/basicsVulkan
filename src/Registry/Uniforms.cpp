//
// Created by migue on 27/07/2026.
//

#include "Uniforms.h"


std::unordered_map<std::string, UniformBuffer*> Uniforms::uniforms;

UniformBuffer* Uniforms::cameraUniform;
Uniforms::LineUBO Uniforms::lineUniform;

