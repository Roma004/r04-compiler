// Purpose: An array size that is not a constant expression is rejected.
int32_t main() {
    int32_t n = 3;
    int32_t [n] a;
    return 0;
}
