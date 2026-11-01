// Purpose: Assigning to a const pointer is rejected.
int32_t main() {
    int32_t x = 1;
    int32_t y = 2;
    int32_t const * p = &x;
    p = &y;
    return 0;
}
