// Purpose: A function pointer cannot be called without dereferencing it.
int32_t inc(int32_t x) {
    return x + 1;
}
int32_t main() {
    int32_t(int32_t) * fp = &inc;
    return fp(1);
}
