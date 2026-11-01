// Purpose: nullptr cannot be passed to an integer parameter.
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    return f(nullptr);
}
