// Purpose: Pointer arithmetic is not supported.
int32_t main() {
    int32_t x = 1;
    int32_t * p = &x;
    int32_t * q = p + 1;
    return 0;
}
