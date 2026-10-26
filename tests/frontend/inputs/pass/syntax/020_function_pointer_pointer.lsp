// Purpose: A pointer to a pointer to a function.
int32_t add(int32_t a) {
    return a;
}
int32_t main() {
    int32_t(int32_t) * fp = &add;
    int32_t(int32_t) ** pp = &fp;
    return 0;
}
