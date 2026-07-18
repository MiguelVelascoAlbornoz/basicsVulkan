#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#include "random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {
   uint64_t time;
   mat4 viewProjectionMatrix;
} ubo;


void main() {
    float timeInSec = float(ubo.time)/1000.0f;
    vec3 randomColor = vec3(randomFloat(worldPos.x+timeInSec),randomFloat(worldPos.y),randomFloat(worldPos.z));
    outColor = vec4(randomColor, 1.0);
}