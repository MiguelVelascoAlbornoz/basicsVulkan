

#version 450
layout(binding = 0) uniform sampler2D sceneColor;
layout(binding = 1) uniform sampler2D sceneDepth;
vec4 sampleTexture(vec2 uv, sampler2D samp){
    return texture(samp,uv);
}

#include "generalPostProcess.glsl"

