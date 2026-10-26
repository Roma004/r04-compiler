// Purpose: Assigning to an element of a const array is rejected.
int32_t main() {
    const int32_t [3] a = {1, 2, 3};
    a[0] = 2;
    return 0;
}
