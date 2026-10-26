// Purpose: Assignment between different integer types is rejected.
int32_t main() {
    int32_t x = 1;
    uint32_t y = 2u;
    x = y;
    return 0;
}
