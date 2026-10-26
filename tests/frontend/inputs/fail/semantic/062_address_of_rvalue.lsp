// Purpose: Taking the address of a non-lvalue expression is rejected.
int32_t main() {
    int32_t * p = &(1 + 2);
    return 0;
}
