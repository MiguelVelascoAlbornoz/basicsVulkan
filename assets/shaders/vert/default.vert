#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;


layout(binding = 2) uniform sampler2D heightMap;
layout(binding = 3) uniform sampler2D displacementMap;

#include "../glsl/CameraUBO.glsl"
#include "../glsl/ModelUBO.glsl"


void main() {
    fragNormal = vec3(modelUBO.rotationMatrix*vec4(normal,1.0f));

    float multiplier = 1;
    float zoom = 1;
    float choppicity = .05;

    vec2 displacementValue = texture(displacementMap,vertexLocalPos.xz*zoom).xy*choppicity;

    worldPos = vec3(modelUBO.modelMatrix*vec4(vertexLocalPos*modelUBO.scale,1.0f));
    worldPos = worldPos + vec3(displacementValue.x,0.0f,displacementValue.y);

    vec4 mapValue = texture(heightMap,vertexLocalPos.xz*zoom)*multiplier;

    float height = (mapValue.x*multiplier);

    vec3 mapNormal = normalize(vec3(-mapValue.z, 1, -mapValue.y));

    worldPos += fragNormal*height;

    fragNormal = mapNormal;
    gl_Position = cameraUBO.viewProjectionMatrix*vec4(worldPos, 1.0);

}