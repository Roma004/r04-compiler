// Purpose: Returning a value of the wrong type is rejected.
int32_t f() {
    return 1u;
}
int32_t main() {
    return 0;
}
