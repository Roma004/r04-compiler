// Purpose: An unterminated character literal is a syntax error.
int32_t main() {
    int8_t c = 'a;
    return 0;
}
