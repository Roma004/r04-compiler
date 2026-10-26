// Purpose: Returning an expression is rejected in a void function.
void_t f() {
    return 1;
}
int32_t main() {
    return 0;
}
