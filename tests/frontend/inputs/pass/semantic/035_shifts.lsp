// Purpose: Shifts keep the left operand type, including on small integers.
int32_t main() {
    uint8_t a = 1uss;
    uint8_t b = 2uss;
    uint8_t c = a << b;
    int32_t x = 1;
    int32_t y = 2;
    int32_t z = x << y;
    int32_t w = x >> y;
    uint64_t u = 1uL;
    uint64_t v = u >> 1;
    return z + w;
}
