// Purpose: A hex literal with no digits is a syntax error.
int32_t main() {
    int32_t x = 0x;
    return 0;
}
