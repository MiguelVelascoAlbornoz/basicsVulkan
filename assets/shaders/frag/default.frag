#version 450
#include "random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragNormal;

layout(std140, binding = 0) uniform UniformBufferObject {

   mat4 viewProjectionMatrix;
   float time; //In seconds
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    mat4 modelMatrix;
    vec3 scale;
    mat4 rotationMatrix;
} modelUBO;

vec3 lightPos = vec3(0.0f,10.0f,2.0f);

void main() {
    vec3 color = vec3(1.0f,0.0f,1.0f);

    vec3 lightDir = normalize(lightPos-worldPos);
    float diffuse = min(max(0,dot(lightDir,fragNormal))+0.1,1);

    vec3 postLightningColor = color*diffuse;
    outColor = vec4(postLightningColor, 1.0);
}