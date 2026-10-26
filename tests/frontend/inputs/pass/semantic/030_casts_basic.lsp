// Purpose: Legal casts between integers, pointers, and arrays.
int32_t main() {
    int32_t x = 1;
    int8_t a = (int8_t)x;
    uint64_t b = (uint64_t)x;
    int32_t * p = &x;
    void_t * v = (void_t *)p;
    int32_t * q = (int32_t *)v;
    uint64_t u = (uint64_t)p;
    int32_t * r = (int32_t *)u;
    int32_t [2] m;
    int8_t [8] bytes = (int8_t [8])m;
    return 0;
}
