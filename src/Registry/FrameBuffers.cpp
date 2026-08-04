//
// Created by migue on 03/08/2026.
//

#include "FrameBuffers.h"

#include "Uniforms.h"
#include "../Renderer/Mesh/FrameBufferObject.h"
#include <algorithm>
std::unordered_map<std::string, FrameBufferObject*> FrameBuffers::frameBuffers;
std::vector<FrameBufferObject*> FrameBuffers::activeFBOs;
FrameBufferObject* FrameBuffers::defaultFrameBuffer;

void FrameBuffers::freeFrameBuffers() {
    for (auto& [name, uniform] : frameBuffers) {
        delete uniform;
    }
    frameBuffers.clear();
}

void FrameBuffers::turnOnFBO(FrameBufferObject* fbo)
{
    FrameBuffers::activeFBOs.push_back(fbo);
}

void FrameBuffers::turnOffFBO(FrameBufferObject* fbo)
{
    auto it = std::find(FrameBuffers::activeFBOs.begin(),  FrameBuffers::activeFBOs.end(), fbo);

    if (it != FrameBuffers::activeFBOs.end())
    {
        FrameBuffers::activeFBOs.erase(it);
    } else
    {
        std::cerr << "Scene not active" << std::endl;
    }
};



