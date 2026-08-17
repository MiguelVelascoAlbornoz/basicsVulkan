#version 450
#include "../glsl/random.glsl"
layout(location = 0) out vec4 outColor;



layout(std140, binding = 0) uniform UniformBufferObject {
   mat4 viewProjectionMatrix;
   float time; //In seconds
} ubo;
layout(std140, push_constant) uniform ModelUBO {
    vec3 direction;
    vec3 color;
} modelUbo;





void main() {


    outColor = vec4(modelUbo.color, 1.0);
}