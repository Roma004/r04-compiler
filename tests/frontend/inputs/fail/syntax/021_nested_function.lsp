// Purpose: A function definition nested inside main is a syntax error.
int32_t main() {
    int32_t inner() {
        return 1;
    }
    return 0;
}
