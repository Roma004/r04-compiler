// Purpose: Taking the address of a function call result is rejected.
int32_t f() {
    return 0;
}
int32_t main() {
    int32_t * p = &f();
    return 0;
}
