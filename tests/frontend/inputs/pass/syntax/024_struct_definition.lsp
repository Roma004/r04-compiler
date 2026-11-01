// Purpose: A struct definition and a variable of that struct type.
struct point_t {
    int32_t x;
    int32_t y;
};
point_t g;
int32_t main() {
    point_t p = [1, 2];
    return p.x;
}
