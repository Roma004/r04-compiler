// Purpose: Casting an expression to void_t is rejected.
int32_t main() {
    int32_t x = 1;
    (void_t)x;
    return 0;
}
