// Purpose: A global initializer that is not a constant expression is rejected.
int32_t f() {
    return 1;
}
int32_t g = f();
int32_t main() {
    return 0;
}
