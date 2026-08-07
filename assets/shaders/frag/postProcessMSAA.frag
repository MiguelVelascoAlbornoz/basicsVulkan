

#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 uv;      // coordenadas UV interpoladas desde el vertex shader
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DMS  sceneColor;

vec4 sampleTexture2DMS()
{
    ivec2 size = textureSize(sceneColor);
    ivec2 pixel = clamp(
        ivec2(uv * vec2(size)),
        ivec2(0),
        size - 1
    );

    int samples = textureSamples(sceneColor);

    vec4 color = vec4(0.0);

    for (int i = 0; i < samples; ++i)
    {
        color += texelFetch(sceneColor, pixel, i);
    }

    return color / float(samples);
}
void main() {
    outColor = vec4(vec3(sampleTexture2DMS().r),1.0f);
}