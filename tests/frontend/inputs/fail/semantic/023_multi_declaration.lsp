// Purpose: Multiple declarations in one statement outside for-init is rejected.
int32_t main() {
    int32_t a = 1, int32_t b = 2;
    return 0;
}
