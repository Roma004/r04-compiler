// Purpose: A parameter ending in a trailing const is a syntax error.
int32_t f(int32_t const) {
    return 0;
}
int32_t main() {
    return 0;
}
