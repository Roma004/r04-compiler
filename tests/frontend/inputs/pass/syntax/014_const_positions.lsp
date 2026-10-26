// Purpose: Every legal placement of const in a declaration.
int32_t main() {
    int32_t x = 1;
    const int32_t a = 1;
    const int32_t *b = &x;
    int32_t const *c = &x;
    const int32_t const *d = &x;
    const int32_t [2] e = [1, 2];
    return 0;
}
