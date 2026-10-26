// Purpose: Assigning to a const scalar is rejected.
int32_t main() {
    const int32_t x = 1;
    x = 2;
    return 0;
}
