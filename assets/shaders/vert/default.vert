#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;


layout(binding = 2) uniform sampler2D heightMap;
layout(binding = 3) uniform sampler2D displacementMap;

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

    vec2 displacementValue = texture(displacementMap,vertexLocalPos.xz).xy*0.00000001;

    worldPos = vec3(modelUBO.modelMatrix*vec4(vertexLocalPos*modelUBO.scale,1.0f));
    worldPos = worldPos + vec3(displacementValue.x,0.0f,displacementValue.y);

    vec4 mapValue = texture(heightMap,vertexLocalPos.xz*1);
    float multiplier = 1.0f;
    float height = (mapValue.x*multiplier);

    vec3 mapNormal = normalize(vec3(-mapValue.z, 1, -mapValue.y));

    worldPos += fragNormal*height;

    fragNormal = mapNormal;

    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}