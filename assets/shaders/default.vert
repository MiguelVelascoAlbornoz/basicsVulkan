#version 450
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

out vec2 fragPos;

void main() {
    fragPos = positions[gl_VertexIndex];
    gl_Position = vec4(fragPos, 0.0, 1.0);
}