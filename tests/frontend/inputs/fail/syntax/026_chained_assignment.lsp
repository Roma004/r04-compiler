// Purpose: Chained assignment is not allowed because assignment is a statement.
int32_t main() {
    int32_t a = 1;
    int32_t b = 2;
    int32_t c = 3;
    a = b = c;
    return 0;
}
