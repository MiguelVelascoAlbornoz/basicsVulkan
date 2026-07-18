int xorshift(int state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float randomFloat(float seed) {
    uint h = hash(floatBitsToUint(seed));
    return float(h) / float(0xffffffffu); // uniforme real en [0,1]
}