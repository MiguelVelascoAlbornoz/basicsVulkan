#version 450

layout (location=0) in  vec3 vertexLocalPos;


#include "../glsl/CameraUBO.glsl"
#include "../glsl/LineModelUBO.glsl"



void main() {
    vec3 worldPos = vertexLocalPos*lineModelUBO.direction;
    gl_Position =cameraUBO.viewProjectionMatrix*vec4(worldPos, 1.0);
}