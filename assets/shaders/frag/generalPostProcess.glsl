layout(location = 0) in vec2 uv;      // coordenadas UV interpoladas desde el vertex shader
layout(location = 0) out vec4 outColor;


void main() {
    outColor = vec4(vec3(sampleTexture(uv,sceneDepth).r),1.0f);
}