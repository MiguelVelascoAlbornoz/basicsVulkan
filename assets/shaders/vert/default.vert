#version 450

layout (location=0) in  vec3 vertexPos;
layout (location=1) in  vec3 color;

layout (location=0) out vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {

    vec3 color;
    float time;
    mat4 viewProjectionMatrix;
} ubo;


void main() {
    fragColor = ubo.color;
    gl_Position = ubo.viewProjectionMatrix*vec4(vertexPos.xy,-10.0, 1.0);
}