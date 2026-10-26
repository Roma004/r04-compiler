// Purpose: Whole-struct assignment from a const struct is rejected.
struct p_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    const p_t a = {1, 2};
    p_t b = {3, 4};
    b = a;
    return 0;
}
