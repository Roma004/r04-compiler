// Purpose: A global list initializer with a non-constant element is rejected.
int32_t f() {
    return 1;
}
int32_t [2] g = {1, f()};
int32_t main() {
    return 0;
}
