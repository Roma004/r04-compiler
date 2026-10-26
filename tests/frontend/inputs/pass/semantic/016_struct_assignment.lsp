// Purpose: Assignment between two values of the same struct type.
struct point_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    point_t a = {1, 2};
    point_t b = {3, 4};
    b = a;
    return b.x;
}
