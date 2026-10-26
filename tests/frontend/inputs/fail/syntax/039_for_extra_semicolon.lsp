// Purpose: A for header with three semicolons is a syntax error.
int32_t main() {
    for (;;;) {
    }
    return 0;
}
