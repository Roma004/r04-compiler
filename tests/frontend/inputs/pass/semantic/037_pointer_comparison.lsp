// Purpose: Pointer equality and inequality comparisons with nullptr.
int32_t main() {
    int32_t x = 1;
    int32_t * p = &x;
    int32_t * q = nullptr;
    int32_t a = p == q;
    int32_t b = p != q;
    return a + b;
}
