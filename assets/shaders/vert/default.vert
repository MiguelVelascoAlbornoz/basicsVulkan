#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 color;

layout (location=0) out vec3 fragColor;
layout (location=1) out vec3 worldPos;

layout(std140, binding = 0) uniform UniformBufferObject {
    uint64_t time;
    mat4 viewProjectionMatrix;
} ubo;


void main() {
    fragColor = color;
    vec3 worldPos = vec3(vertexLocalPos.xy,6.0f);
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}