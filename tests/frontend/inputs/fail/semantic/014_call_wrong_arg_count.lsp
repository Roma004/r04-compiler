// Purpose: A call with the wrong number of arguments is rejected.
int32_t f(int32_t a) {
    return a;
}
int32_t main() {
    return f();
}
