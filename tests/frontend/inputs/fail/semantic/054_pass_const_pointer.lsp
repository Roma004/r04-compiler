// Purpose: Passing a pointer to const to a non-const pointer parameter is rejected.
int32_t f(int32_t * p) {
    return *p;
}
int32_t main() {
    const int32_t x = 1;
    const int32_t * q = &x;
    return f(q);
}
