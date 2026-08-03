//
// Created by migue on 03/08/2026.
//

#include "FrameBuffers.h"

#include "Uniforms.h"
#include "../Renderer/Mesh/FrameBufferObject.h"

std::unordered_map<std::string, FrameBufferObject*> FrameBuffers::frameBuffers;

FrameBufferObject* FrameBuffers::defaultFrameBuffer;

void FrameBuffers::freeFrameBuffers() {
    for (auto& [name, uniform] : frameBuffers) {
        delete uniform;
    }
    frameBuffers.clear();
};

