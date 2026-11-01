// Purpose: A void_t function taking a pointer parameter.
void_t set(int32_t * p, int32_t v) {
    *p = v;
    return;
}
int32_t main() {
    int32_t x = 0;
    set(&x, 5);
    return x;
}
