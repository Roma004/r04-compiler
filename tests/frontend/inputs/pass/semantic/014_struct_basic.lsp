// Purpose: A basic struct definition, declaration, and field access.
struct point_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    point_t p;
    p.x = 1;
    p.y = 2;
    return p.x + p.y;
}
