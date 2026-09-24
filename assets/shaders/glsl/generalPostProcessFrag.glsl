

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

#include "random.glsl"
#extension GL_EXT_control_flow_attributes : enable
struct Particle {
    float charge;
    vec2 pos;
};

const int particlesCount = 200;
const float particlesRadius = 1.0;
const vec3 particlesColor = vec3(1.0, 0.0, 0.0);
const vec3 linesColor = vec3(0.0f,1.0f,0.0f);

void main() {

    Particle particles[particlesCount];
    float totalCharge = 0;
    [[dont_unroll]]
    for (int i = 0; i < particlesCount; ++i){

        float angle = (3.14*2/particlesCount)*i;
        particles[i].pos = vec2(sin(angle),cos(angle))*100+vec2(500,300);

        particles[i].charge = -sin(particles[i].pos.x/10.0f)*(10)/float(particlesCount);
        totalCharge += particles[i].charge;
    }

    outColor = vec4(0.0);\
    vec2 pixelCoord = gl_FragCoord.xy;

    float pot = 0.0;
    float psi = 0.0;
    [[dont_unroll]]
    for (int i = 0; i < particlesCount; ++i) {
        vec2 directionToParticle = particles[i].pos - pixelCoord;
        float r2 = dot(directionToParticle, directionToParticle);
        if (r2 < particlesRadius * particlesRadius) { outColor = vec4(1,0,0,1); return; }
        pot += particles[i].charge * inversesqrt(r2);
        //psi += particles[i].charge * atan(directionToParticle.y, directionToParticle.x);     // ángulo visto desde ESTA carga
        psi += particles[i].charge * directionToParticle.x * inversesqrt(dot(directionToParticle, directionToParticle));
    }


    float spacing = 0.05;              // separación entre líneas, ajusta a ojo
    float dpsi = fwidth(psi);
    float f = psi / spacing;
    float dist = abs(fract(f + 0.5) - 0.5) * spacing / max(dpsi, 1e-5);
    float line = 1.0 - smoothstep(0.0, 1.5, dist);

    // donde el gradiente es anormalmente grande, hay un salto de rama: apaga la línea
    float jump = step(dpsi, spacing * 4.0); // ajusta el 4.0 a ojo
    line *= jump;

    float m = sign(pot) * log(1.0 + abs(pot) * 50.0);
    vec3 col = vec3(max(m, 0.0), 0.0, max(-m, 0.0));
    col = mix(col, linesColor, line);
    outColor = vec4(col, 1.0);

    //float m = log(1.0 + a * 1);        // 1. logarítmico (símlog): el más usado
    // float m = 1.0 - exp(-a * k);    // 2. saturante suave, acotado a 0..1
    // float m = tanh(a * k);          // 3. parecido al 2, acotado a 0..1

}