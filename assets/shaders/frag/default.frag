#version 450
#include "../glsl/random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;


layout(binding = 0) uniform sampler2D image;



vec3 lightPos = vec3(0.0f,10.0f,0);

void main() {
    outColor = vec4(texture(image,inUV).xyz,1.0f);
}