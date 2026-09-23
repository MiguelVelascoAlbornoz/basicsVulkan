#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;

#include "../glsl/CameraUBO.glsl"
#include "../glsl/ModelUBO.glsl"
void main() {
    fragNormal = vec3(modelUBO.rotationMatrix*vec4(normal,1.0f));
    worldPos = vec3(modelUBO.modelMatrix*vec4(vertexLocalPos*modelUBO.scale,1.0f));
    gl_Position = cameraUBO.viewProjectionMatrix*vec4(worldPos, 1.0);
}