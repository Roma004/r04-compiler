// Purpose: Repeated identical prototypes followed by a single definition.
int32_t square(int32_t x);
int32_t square(int32_t x);
int32_t square(int32_t x) {
    return x * x;
}
int32_t main() {
    return square(3);
}
