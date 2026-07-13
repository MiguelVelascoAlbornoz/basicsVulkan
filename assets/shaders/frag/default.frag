#version 450

layout(location = 0) out vec4 outColor;
layout (location = 0) in vec2 fragPos;

void main() {
    
    outColor = vec4(fragPos.x*0.5+0.5, fragPos.y*0.5+0.5, 0.0, 1.0);
}