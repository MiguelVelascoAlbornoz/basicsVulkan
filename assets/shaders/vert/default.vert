#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;


void main() {
    fragNormal = normal;
    vec3 worldPos = vertexLocalPos;
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}