

#version 450

layout(location = 0) in vec2 uv;      // coordenadas UV interpoladas desde el vertex shader
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sceneColor;

void main() {
    outColor = texture(sceneColor, uv);
}