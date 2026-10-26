// Purpose: Signed overflow in a constant expression is a compile-time error.
int32_t g = 2147483647 + 1;
int32_t main() {
    return 0;
}
