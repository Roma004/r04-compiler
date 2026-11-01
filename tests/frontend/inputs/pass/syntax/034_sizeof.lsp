// Purpose: sizeof applied to types and to expressions.
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    int32_t x = 1;
    uint64_t a = sizeof(int32_t);
    uint64_t b = sizeof(int32_t *);
    uint64_t c = sizeof(int32_t [3]);
    uint64_t d = sizeof(x);
    uint64_t e = sizeof(f(x));
    return 0;
}
