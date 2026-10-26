// Purpose: Comparing a pointer with integer zero is rejected.
int32_t main() {
    int32_t x = 1;
    int32_t * p = &x;
    if (p == 0) {
        return 1;
    }
    return 0;
}
