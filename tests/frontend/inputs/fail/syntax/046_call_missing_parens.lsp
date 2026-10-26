// Purpose: A function call written without parentheses is a syntax error.
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    f 1;
    return 0;
}
