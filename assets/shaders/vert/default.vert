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
    vec2 a[FFT_N][FFT_N];
    vec2 A[FFT_N][FFT_N];
    //vec2 dA[FFT_N];
    //vec2 da[FFT_N];
    for (int i = 0; i < FFT_N; ++i){
        for (int j = 0; j < FFT_N ; ++j){
            a[i][j] = vec2(0.0f,0.0f);
        }

        //da[i] = vec2(0.0f,0.0f);
    }
    // Par conjugado: única forma de obtener un coseno puro real
    setWave(a,2,ivec2(10,10),0);

    for (int i = 0; i < FFT_N; ++i){
        ifftColumn(a, A, FFT_LOG2N,i);
    }
    for (int i = 0; i < FFT_N; ++i){
        ifftFila(A, a, FFT_LOG2N,i);
    }

    //ifft(da,dA,FFT_LOG2N);

    vec2 waveDir = normalize(vec2(1.0f,1.0f));
    vec2 u = vec2(mod((vertexLocalPos.x)*FFT_N*1,FFT_N),mod((vertexLocalPos.z)*FFT_N*1,FFT_N));
    ivec2 i0 = ivec2(u.x,u.y);
    ivec2 i1;
    i1.x = i0.x == FFT_N-1 ? 0 : i0.x+1;
    i1.y = i0.y == FFT_N-1 ? 0 : i0.y+1;
    vec2 t = fract(u);

    float height = mix(a[i0.x][i0.y].x,a[i1.x][i1.y].x,t.x);

    //float d = mix(dA[i0].x,dA[i1].x,t);

    //vec3 fftNormal = normalize(cross(vec3(0.0f,0.0f,1.0f),(normalize(vec3(d,0.0f,0.0f)))));

    fragNormal = vec3(modelUBO.rotationMatrix * vec4(normal, 1.0f));
    vec3 worldPos = vec3(modelUBO.modelMatrix * vec4(vertexLocalPos * modelUBO.scale, 1.0f))
    + normal * height;
    gl_Position = ubo.viewProjectionMatrix * vec4(worldPos, 1.0);
}