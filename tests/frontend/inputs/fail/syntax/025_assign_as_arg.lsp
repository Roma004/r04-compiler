// Purpose: Assignment cannot be used as a function argument expression.
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    int32_t a = 1;
    int32_t b = 2;
    return f(a = b);
}
