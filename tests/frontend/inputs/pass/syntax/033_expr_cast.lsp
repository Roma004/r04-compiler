// Purpose: Casts between integers, pointers, functions, and equal-sized arrays.
struct a_t {
    int32_t x;
};
int32_t main() {
    int32_t x = 1;
    int8_t a = (int8_t)x;
    uint64_t b = (uint64_t)x;
    int32_t * p = (int32_t *)x;
    void_t * v = (void_t *)p;
    const int32_t * cp = (const int32_t *)p;
    int32_t [2][2] arr;
    int8_t [16] bytes = (int8_t [16])arr;
    int32_t(int32_t) * fp = nullptr;
    int32_t * dp = (int32_t *)fp;
    int32_t(int32_t) * fp2 = (int32_t(int32_t) *)dp;
    a_t value;
    const a_t cvalue = (const a_t)value;
    return 0;
}
