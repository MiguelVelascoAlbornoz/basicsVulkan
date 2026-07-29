#version 450
#include "random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {
   mat4 viewProjectionMatrix;
   float time; //In seconds
} ubo;
layout(std140, push_constant) uniform ModelUBO {
    vec3 direction;
    vec3 color;
} modelUbo;



void main() {
    outColor = vec4(modelUbo.color, 1.0);
}