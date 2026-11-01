// Purpose: A bare return is rejected in a non-void function.
int32_t f() {
    return;
}
int32_t main() {
    return 0;
}
