#version 450
#include "../glsl/random.glsl"
#include "../glsl/LineModelUBO.glsl"

layout(location = 0) out vec4 outColor;

void main() {


    outColor = vec4(lineModelUBO.color, 1.0);
}