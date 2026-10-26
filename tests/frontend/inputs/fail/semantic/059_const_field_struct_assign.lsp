// Purpose: Whole-struct assignment is rejected when a field is const.
struct p_t {
    const int32_t x;
    int32_t y;
};
int32_t main() {
    p_t a = {1, 2};
    p_t b = {3, 4};
    a = b;
    return 0;
}
