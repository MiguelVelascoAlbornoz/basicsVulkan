#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout (location=0) in  vec3 vertexPos;
layout (location=1) in  vec3 color;

layout (location=0) out vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {
    uint64_t time;
    mat4 viewProjectionMatrix;
} ubo;


void main() {
    fragColor = color;
    gl_Position = ubo.viewProjectionMatrix*vec4(vertexPos.xy+vec2(0.0f,0.0f).xy,-6.0, 1.0);
}