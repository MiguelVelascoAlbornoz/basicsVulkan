#version 450

layout (location=0) in  vec3 vertexLocalPos;
layout (location=1) in  vec3 normal;

layout (location=0) out vec3 fragNormal;
layout (location=1) out vec3 worldPos;

layout(std140, binding = 0) uniform UniformBufferObject {

    mat4 viewProjectionMatrix;
    float time;
} ubo;
layout(std140,push_constant) uniform ModelUBO {
    mat4 modelMatrix;
    vec3 scale;
    mat4 rotationMatrix;
} modelUBO;


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

uint bitReverse(uint x, uint log2n)
{
    uint n = 0u;
    for (uint i = 0u; i < log2n; ++i) {
        n <<= 1u;
        n |= (x & 1u);
        x >>= 1u;
    }
    return n;
}

vec2 complexExp(float angle)
{
    return vec2(cos(angle), sin(angle));
}

vec2 complexMul(vec2 a, vec2 b)
{
    return vec2(a.x * b.x - a.y * b.y,
    a.x * b.y + a.y * b.x);
}

const float PI = 3.14159265358979323846;

// Función iterativa para la FFT (Cooley-Tukey, in-place tras bit-reversal)
void fft(vec2 a[100],out vec2 A[100], uint log2n)
{
    uint n = 1u << log2n; // tamaño real de la FFT, derivado de log2n

    // bit reversal of the given array
    for (uint i = 0u; i < n; ++i) {
        uint rev = bitReverse(i, log2n);
        A[i] = a[rev];
    }

    for (uint s = 1u; s <= log2n; ++s)
    {
        uint m  = 1u << s;
        uint m2 = m >> 1u;

        float angle = -2.0 * PI / float(m);
        vec2 wm = complexExp(angle);

        for (uint j = 0u; j < m2; ++j)
        {
            vec2 w = complexExp(angle * float(j)); // w = wm^j, evita acumular error

            for (uint k = j; k < n; k += m)
            {
                vec2 t = complexMul(w, A[k + m2]);
                vec2 u = A[k];

                A[k]      = u + t;
                A[k + m2] = u - t;
            }
        }
    }
}
const uint FFT_N = 8u;
const uint FFT_LOG2N = 3u; // log2(8) = 3

void main() {
    vec2 a[100];
    vec2 A[100];

    a[0] = vec2(0.0, 0.0);
    a[1] = vec2(1.0, 0.0);
    a[2] = vec2(2.0, 0.0);
    a[3] = vec2(3.0, 0.0);
    a[4] = vec2(4.0, 0.0);
    a[5] = vec2(5.0, 0.0);
    a[6] = vec2(6.0, 0.0);
    a[7] = vec2(7.0, 1.0);

    fft(a, A, FFT_LOG2N);

    // Mapea vertexLocalPos.x en [-0.5, 0.5] a un índice en [0, N-1]
    int index = int(round((vertexLocalPos.x + 0.5) * float(FFT_N - 1u)));
    index = clamp(index, 0, int(FFT_N) - 1);

    float height = A[index].x; // parte real como altura

    fragNormal = vec3(modelUBO.rotationMatrix * vec4(normal, 1.0f));
    vec3 worldPos = vec3(modelUBO.modelMatrix * vec4(vertexLocalPos * modelUBO.scale, 1.0f))
    + fragNormal * height;
    gl_Position = ubo.viewProjectionMatrix * vec4(worldPos, 1.0);
}