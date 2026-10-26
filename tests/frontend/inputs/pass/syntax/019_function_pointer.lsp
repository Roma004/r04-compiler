// Purpose: A variable holding a single function pointer.
int32_t add(int32_t a, int32_t b) {
    return a + b;
}
int32_t main() {
    int32_t(int32_t, int32_t) *fp = &add;
    return 0;
}
