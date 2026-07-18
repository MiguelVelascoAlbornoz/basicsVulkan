#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) out vec4 outColor;
layout (location = 0) in vec3 fragColor;

layout(std140, binding = 0) uniform UniformBufferObject {
   uint64_t time;
   mat4 viewProjectionMatrix;
} ubo;

void main() {
    float timeInSec = float(ubo.time)/1000.0f;
    outColor = vec4(fragColor*vec3(sin(timeInSec)*0.5+0.5,cos(timeInSec)*0.5+0.5,sin(timeInSec+1)*0.5+0.5), 1.0);
}