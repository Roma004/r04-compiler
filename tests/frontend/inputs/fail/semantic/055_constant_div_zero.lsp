// Purpose: Division by zero in a constant expression is a compile-time error.
int32_t g = 1 / 0;
int32_t main() {
    return 0;
}
