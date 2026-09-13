layout(location = 0) in vec2 uv;      // coordenadas UV interpoladas desde el vertex shader
layout(location = 0) out vec4 outColor;

#include "random.glsl"

void main() {
    outColor = vec4(vec3(sampleTexture(uv,sceneColor)),1.0f);

    float dither = randomFloat(uv.x*124567+uv.y*1456789);
    outColor.rgb += dither*0.001;
}