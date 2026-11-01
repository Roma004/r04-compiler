// Purpose: A function call with two arguments missing a comma is a syntax error.
int32_t f(int32_t a, int32_t b) {
    return a + b;
}
int32_t main() {
    return f(1 2);
}
