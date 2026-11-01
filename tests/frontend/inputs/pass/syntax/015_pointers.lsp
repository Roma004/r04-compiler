// Purpose: Single, double, and triple pointer types.
int32_t main() {
    int32_t x = 1;
    int32_t * a = &x;
    int32_t ** b = &a;
    int32_t *** c = &b;
    return 0;
}
