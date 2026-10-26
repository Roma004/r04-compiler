// Purpose: A function call missing its closing parenthesis is a syntax error.
int32_t f(int32_t x) {
    return x;
}
int32_t main() {
    return f(1;
}
