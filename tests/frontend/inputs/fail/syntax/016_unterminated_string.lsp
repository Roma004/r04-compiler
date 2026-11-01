// Purpose: An unterminated string literal is a syntax error.
int32_t main() {
    const int8_t * const s = "abc;
    return 0;
}
