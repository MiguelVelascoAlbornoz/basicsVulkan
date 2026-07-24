#version 450
#include "random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {

   mat4 viewProjectionMatrix;
} ubo;


void main() {
   // float timeInSec = ubo.time;
   // vec3 randomColor = vec3(randomFloat(round(worldPos.x*100+10+worldPos.y*100)),randomFloat(round(worldPos.x*100+5678+worldPos.y*100)),randomFloat(round(worldPos.x*100+678+worldPos.y*100)));
    outColor = vec4(vec3(1.0f,0.0f,0.0f), 1.0);
}