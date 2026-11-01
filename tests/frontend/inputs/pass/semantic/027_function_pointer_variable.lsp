// Purpose: Function pointer variables and a function returning a function pointer.
int32_t inc(int32_t x) {
    return x + 1;
}
int32_t(int32_t) * choose() {
    return &inc;
}
int32_t main() {
    int32_t(int32_t) * fp = &inc;
    int32_t(int32_t) * chosen = choose();
    return (*fp)(1) + (*chosen)(1);
}
