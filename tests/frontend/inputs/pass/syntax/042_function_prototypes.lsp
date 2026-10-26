// Purpose: Repeated identical prototypes followed by one definition.
int32_t f(int32_t x);
int32_t f(int32_t x);
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    return f(1);
}
