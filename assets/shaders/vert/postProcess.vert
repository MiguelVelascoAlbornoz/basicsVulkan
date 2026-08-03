#version 450

layout (location=0) in  vec2 vertexLocalPos;

// vertex shader del postProcess, opción sin tocar el mesh
layout(location = 0) out vec2 uv;


void main() {
        uv = (vertexLocalPos + 1.0) * 0.5; // si inPosition va de -1 a 1
        uv.y = 1.0 - uv.y; // depende de tu convención de Y

    gl_Position = vec4(vertexLocalPos,0.0f, 1.0);
}