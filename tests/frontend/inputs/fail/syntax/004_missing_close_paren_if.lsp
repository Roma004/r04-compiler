// Purpose: An if condition missing its closing parenthesis is a syntax error.
int32_t main() {
    int32_t x = 1;
    if (x {
        return 1;
    }
    return 0;
}
