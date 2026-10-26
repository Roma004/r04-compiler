// Purpose: A function may return a struct by value.
struct point_t {
    int32_t x;
    int32_t y;
};
point_t origin() {
    point_t p = {0, 0};
    return p;
}
int32_t main() {
    point_t o = origin();
    return o.x;
}
