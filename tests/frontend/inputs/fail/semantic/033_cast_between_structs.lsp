// Purpose: Casting between different struct types is rejected.
struct a_t {
    int32_t x;
};
struct b_t {
    int32_t y;
};
int32_t main() {
    a_t a = {1};
    b_t b = (b_t)a;
    return 0;
}
