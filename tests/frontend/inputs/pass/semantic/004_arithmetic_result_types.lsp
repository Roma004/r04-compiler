// Purpose: Mixed-width arithmetic produces the documented result types.
int32_t main() {
    uint8_t a = 1uss;
    uint8_t b = 2uss;
    uint8_t c = a + b;
    uint8_t d = 3uss;
    int32_t e = 4;
    int32_t f = d + e;
    uint32_t g = 5u;
    int32_t h = 6;
    uint32_t i = g + h;
    uint64_t j = 7uL;
    int64_t k = 8L;
    uint64_t l = j + k;
    int64_t m = 9L;
    uint32_t n = 10u;
    int64_t o = m + n;
    return c + f;
}
