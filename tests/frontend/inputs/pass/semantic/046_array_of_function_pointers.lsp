// Purpose: An array of function pointers is initialized and called through.
int32_t one(int32_t x) {
    return x + 1;
}
int32_t two(int32_t x) {
    return x + 2;
}
int32_t main() {
    int32_t(int32_t) *[2] fs = {&one, &two};
    return (*fs[0])(1);
}
