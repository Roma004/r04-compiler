// Purpose: Pointer comparisons other than == and != are rejected.
int32_t main() {
    int32_t x = 1;
    int32_t * p = &x;
    int32_t * q = &x;
    return p < q;
}
