// Purpose: The comma operator is not an expression operator.
int32_t main() {
    int32_t x = 1;
    int32_t y = 2;
    int32_t z = (x, y);
    return z;
}
