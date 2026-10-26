// Purpose: Assignment is a statement and cannot be an if condition expression.
int32_t main() {
    int32_t x = 1;
    if (x = 5) {
        return 1;
    }
    return 0;
}
