// Purpose: Struct initialization with a list of field values.
struct point_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    point_t p = {3, 4};
    return p.x;
}
