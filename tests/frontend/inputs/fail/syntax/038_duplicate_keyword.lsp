// Purpose: A duplicated if keyword is a syntax error.
int32_t main() {
    int32_t x = 1;
    if if (x) {
        return 1;
    }
    return 0;
}
