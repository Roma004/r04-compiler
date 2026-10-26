// Purpose: Whole-array assignment to a const array is rejected.
int32_t main() {
    const int32_t [2] a = {1, 2};
    int32_t [2] b = {3, 4};
    a = b;
    return 0;
}
