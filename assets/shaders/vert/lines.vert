#version 450

layout (location=0) in  vec3 vertexLocalPos;


layout (location=0) out vec3 fragColor;
layout (location=1) out vec3 worldPos;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    vec3 direction;
    vec3 color;
} modelUBO;



void main() {
    fragColor = vec3(1.0f);
    vec3 worldPos = vertexLocalPos*modelUBO.direction;
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}