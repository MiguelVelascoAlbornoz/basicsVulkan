#version 450

layout(location = 0) out vec4 outColor;
layout (location = 0) in vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {
   
   vec3 color;
   float time;
} ubo;

void main() {
    
    outColor = vec4(fragColor, 1.0);
}