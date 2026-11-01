// Purpose: Pass and call a function pointer parameter.
int32_t inc(int32_t x) {
    return x + 1;
}
int32_t apply(int32_t(int32_t) * f, int32_t v) {
    return (*f)(v);
}
int32_t main() {
    return apply(&inc, 4);
}
