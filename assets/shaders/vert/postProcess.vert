#version 450

layout (location=0) in  vec2 vertexLocalPos;



void main() {

    gl_Position = vec4(vertexLocalPos,0.0f, 1.0);
}