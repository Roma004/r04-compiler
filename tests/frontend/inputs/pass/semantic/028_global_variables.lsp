// Purpose: Globals with scalar, array, and struct initializers.
int32_t g1 = 5;
uint32_t g2 = 7u;
int32_t g3;
int32_t [3] ga = {1, 2, 3};
struct point_t {
    int32_t x;
    int32_t y;
};
point_t gp = {1, 2};
int32_t main() {
    return g1 + ga[0] + gp.x;
}
