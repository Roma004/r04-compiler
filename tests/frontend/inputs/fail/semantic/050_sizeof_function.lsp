// Purpose: sizeof of a function is rejected.
int32_t f() {
    return 0;
}
int32_t main() {
    uint64_t n = sizeof(f);
    return 0;
}
