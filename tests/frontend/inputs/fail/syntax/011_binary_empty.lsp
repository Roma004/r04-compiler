// Purpose: A binary literal with no digits is a syntax error.
int32_t main() {
    int32_t x = 0b;
    return 0;
}
