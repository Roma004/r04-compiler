// Purpose: Redeclaring a parameter in the function's top-level block is rejected.
int32_t f(int32_t x) {
    int32_t x = 2;
    return x;
}
int32_t main() {
    return 0;
}
