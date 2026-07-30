#version 450

layout (location=0) in  vec3 vertexLocalPos;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    vec3 direction;
    vec3 color;
} modelUBO;



void main() {
    vec3 worldPos = vertexLocalPos*modelUBO.direction;
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}