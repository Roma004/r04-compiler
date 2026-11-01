// Purpose: Dereferencing a non-pointer value is rejected.
int32_t main() {
    int32_t x = 1;
    return *x;
}
