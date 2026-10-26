// Purpose: sizeof on types, variables, and nullptr yields uint64_t.
struct pair_t {
    int32_t a;
    int32_t b;
};
int32_t main() {
    int32_t x = 1;
    uint64_t a = sizeof(int32_t);
    uint64_t b = sizeof(x);
    uint64_t c = sizeof(int32_t *);
    uint64_t d = sizeof(pair_t);
    uint64_t e = sizeof(void_t);
    uint64_t f = sizeof(nullptr);
    return 0;
}
