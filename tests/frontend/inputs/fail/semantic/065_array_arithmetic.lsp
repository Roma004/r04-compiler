// Purpose: Using an array as a whole in arithmetic is rejected.
int32_t main() {
    int32_t [2] a = {1, 2};
    int32_t x = a + 1;
    return 0;
}
