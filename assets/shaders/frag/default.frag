#version 450
#include "../glsl/random.glsl"
layout(location = 0) out vec4 outColor;

layout (location = 1) in vec3 worldPos;
layout (location = 0) in vec3 fragNormal;

layout(binding = 1) uniform sampler2D image;

#include "../glsl/CameraUBO.glsl"

vec3 lightPos = vec3(0.0f,10.0f,0);

void main() {
    vec3 color = texture(image,worldPos.xz).xyz;

    vec3 lightDir = normalize(lightPos-worldPos);

    vec3 playerDir = (cameraUBO.viewPos-worldPos);
    float distanceToPlayer = length(playerDir);
    playerDir = normalize(playerDir);

    //DIFFUSE
    float diffuse = min(max(0,dot(lightDir,fragNormal)),1);
    //SPECULAR
    float shininess = 50;
    vec3 R = reflect(-lightDir, fragNormal);
    float specular = pow(max(dot(R, playerDir), 0.0), shininess);
    //Aproximacion de fresnel
    float VdotN = max(dot(playerDir, fragNormal), 0.0);
    float F0 = 0.02; // Reflectancia base del agua (índice de refracción ~1.33)
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotN, 5.0);
    float finalSpecular = specular;

    //Aplicacion de lightning
    vec3 ambLightning = vec3(1.0f,1.0f,0.0f);
    vec3 postLightningColor = color * (ambLightning + diffuse) + finalSpecular;
    outColor = vec4(postLightningColor, 1.0);
}