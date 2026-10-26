// Purpose: A call with an argument of the wrong type is rejected.
int32_t f(int32_t a) {
    return a;
}
int32_t main() {
    return f(1u);
}
