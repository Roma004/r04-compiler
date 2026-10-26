// Purpose: A function whose return type is a pointer to a function.
int32_t add(int32_t a) {
    return a;
}
int32_t(int32_t) * choose(int32_t which) {
    return &add;
}
int32_t main() {
    int32_t(int32_t) * fp = choose(0);
    return (*fp)(1);
}
