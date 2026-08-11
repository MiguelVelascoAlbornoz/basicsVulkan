#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;


layout(binding = 2) uniform sampler2D heightMap;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    mat4 modelMatrix;
    vec3 scale;
    mat4 rotationMatrix;
} modelUBO;



void main() {
    fragNormal = vec3(modelUBO.rotationMatrix*vec4(normal,1.0f));
    worldPos = vec3(modelUBO.modelMatrix*vec4(vertexLocalPos*modelUBO.scale,1.0f));

    float height = texture(heightMap,vertexLocalPos.xz).x*1000;
    worldPos += fragNormal*height;

    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}