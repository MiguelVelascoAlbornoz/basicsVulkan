#version 450

layout (location=0) in  vec2 vertexLocalPos;


layout (location=0) out vec3 fragColor;
layout (location=1) out vec3 worldPos;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;


void main() {
    fragColor = vec3(1.0f);
    vec3 worldPos = vec3(vertexLocalPos.xy,1.f);
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}