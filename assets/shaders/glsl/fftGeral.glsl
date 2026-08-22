float L = 1.0f;
const float PI = 3.14159265358979323846;
const uint FFT_N = 512;
ivec2 kToIndex(vec2 k) {

    int col = int(round(k.x));
    int fil = int(round(k.y));

    // wraparound a [0, N-1]
    col = (col + int(FFT_N)) % int(FFT_N);
    fil = (fil + int(FFT_N)) % int(FFT_N);

    return ivec2(col, fil);
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


vec2 calculateK(uint fila, uint column){

    int iiCol = (column <= FFT_N/2) ? int(column) : int(column - FFT_N);
    int iiFil = (fila    <= FFT_N/2) ? int(fila)    : int(fila    - FFT_N);


    return vec2(iiCol, iiFil); // AHORA sí, esto es tu vector de frecuencia 2D
}
