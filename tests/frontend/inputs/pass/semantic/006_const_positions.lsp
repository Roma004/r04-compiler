// Purpose: Exercise every legal const placement in declarations.
int32_t main() {
    int32_t x = 1;
    const int32_t a = 1;
    const int32_t * b = &x;
    int32_t const * c = &x;
    const int32_t const * d = &x;
    const int32_t [3] arr = {1, 2, 3};
    return a;
}
