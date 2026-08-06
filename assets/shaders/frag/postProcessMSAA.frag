

#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 uv;      // coordenadas UV interpoladas desde el vertex shader
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DMS  sceneColor;

vec4 sampleTexture2DMS(){
    ivec2 size = textureSize(sceneColor);
    ivec2 pixel = clamp(
        ivec2(uv * vec2(size)),
        ivec2(0),
        size - 1
    );
    vec4 c0 = texelFetch(sceneColor, pixel, 0);
    vec4 c1 = texelFetch(sceneColor, pixel, 1);
    vec4 c2 = texelFetch(sceneColor, pixel, 2);
    vec4 c3 = texelFetch(sceneColor, pixel, 3);

     return  (c0 + c1 + c2 + c3) * 0.25;

}

void main() {
    outColor = vec4(vec3(sampleTexture2DMS().r),1.0f);
}