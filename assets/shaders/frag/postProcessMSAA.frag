

#version 450
layout(binding = 0) uniform sampler2DMS  sceneColor;
layout(binding = 1) uniform sampler2DMS sceneDepth;
vec4 sampleTexture(vec2 uv, sampler2DMS samp)
{
    ivec2 size = textureSize(samp);
    ivec2 pixel = clamp(
        ivec2(uv * vec2(size)),
        ivec2(0),
        size - 1
    );

    int samples = textureSamples(samp);

    vec4 color = vec4(0.0);

    for (int i = 0; i < samples; ++i)
    {
        color += texelFetch(samp, pixel, i);
    }

    return color / float(samples);
}
#include "generalPostProcess.glsl"


