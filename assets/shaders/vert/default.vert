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
const uint FFT_N = 32u;
const uint FFT_LOG2N = 5u; // log2(8) = 3

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
void ifftColumn(vec2 a[FFT_N][FFT_N], inout vec2 A[FFT_N][FFT_N], uint log2n, int column)
{
    uint n = 1u << log2n;
    float invN = 1.0 / float(n);

    // bit reversal + normalización fusionadas en el mismo recorrido
    for (uint i = 0u; i < n; ++i) {
        uint rev = bitReverse(i, log2n);
        A[column][i] = a[column][rev] * invN;
    }

    for (uint s = 1u; s <= log2n; ++s)
    {
        uint m  = 1u << s;
        uint m2 = m >> 1u;
        float angle = 2.0 * PI / float(m);

        for (uint j = 0u; j < m2; ++j)
        {
            vec2 w = complexExp(angle * float(j));
            for (uint k = j; k < n; k += m)
            {
                vec2 t = complexMul(w, A[column][k + m2]);
                vec2 u = A[column][k];
                A[column][k]      = u + t;
                A[column][k + m2] = u - t;
            }
        }
    }
}
void ifftFila(vec2 a[FFT_N][FFT_N], inout vec2 A[FFT_N][FFT_N], uint log2n, int fila)
{
    uint n = 1u << log2n;
    float invN = 1.0 / float(n);

    // bit reversal + normalización fusionadas en el mismo recorrido
    for (uint i = 0u; i < n; ++i) {
        uint rev = bitReverse(i, log2n);
        A[i][fila] = a[rev][fila] * invN;
    }

    for (uint s = 1u; s <= log2n; ++s)
    {
        uint m  = 1u << s;
        uint m2 = m >> 1u;
        float angle = 2.0 * PI / float(m);

        for (uint j = 0u; j < m2; ++j)
        {
            vec2 w = complexExp(angle * float(j));
            for (uint k = j; k < n; k += m)
            {
                vec2 t = complexMul(w, A[(k+m2)][fila]);
                vec2 u = A[k][fila];
                A[k][fila]      = u + t;
                A[(k+m2)][fila] = u - t;
            }
        }
    }
}

void setWave(inout vec2 a[FFT_N][FFT_N], float amp, ivec2 k, float phase)
{
    ivec2 i0 = k;
    ivec2 i1;
    i1.x = int((FFT_N - k.x) % FFT_N);
    i1.y = int((FFT_N - k.y) % FFT_N);
    a[i0.x][i0.y] = vec2(amp * cos(phase), amp*sin(phase));
    a[i1.x][i1.y] = vec2(amp * cos(phase), -amp*sin(phase));

    //float k1 = 2.0 * PI * float(k) / float(FFT_N);
    //float k0 = -k1;

    //da[i1] = complexMul(a[i1], vec2(0.0, k1));
    //da[i0] = complexMul(a[i0], vec2(0.0, k0));
}

void main() {
    fragNormal = vec3(modelUBO.rotationMatrix*vec4(normal,1.0f));
    worldPos = vec3(modelUBO.modelMatrix*vec4(vertexLocalPos*modelUBO.scale,1.0f));
    gl_Position = ubo.viewProjectionMatrix*vec4(worldPos, 1.0);
}