#version 450
#include "../glsl/random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragNormal;

layout(binding = 1) uniform sampler2D image;

layout(std140, binding = 0) uniform UniformBufferObject {

   mat4 viewProjectionMatrix;
   float time; //In seconds
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    mat4 modelMatrix;
    vec3 scale;
    mat4 rotationMatrix;
} modelUBO;

vec3 lightPos = vec3(0.0f,15.0f,15.0f);

void main() {
    vec3 color = texture(image,worldPos.xz).xyz;

    vec3 lightDir = normalize(lightPos-worldPos);
    float diffuse = min(max(0,dot(lightDir,fragNormal))+0.1,1);

    vec3 postLightningColor = color*diffuse;
    outColor = vec4(postLightningColor, 1.0);
}