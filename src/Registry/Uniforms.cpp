//
// Created by migue on 27/07/2026.
//

#include "Uniforms.h"
#include "../renderer/UniformBuffer.h"

std::unordered_map<std::string, UniformBuffer*> Uniforms::uniforms;

void Uniforms::freeUniforms(){
        for (auto& [name, uniform] : uniforms) {
                delete uniform;
        }
        uniforms.clear();
};

UniformBuffer* Uniforms::cameraUniform;

void Uniforms::onPlayerRenderUpdate() {

        Uniforms::cameraUniform->addIndexToQueue(CameraUBO::VIEW_PROJECTION_MATRIX);
        Uniforms::cameraUniform->addIndexToQueue(CameraUBO::VIEW_POS);

}

Uniforms::LineUBO Uniforms::lineUniform;

