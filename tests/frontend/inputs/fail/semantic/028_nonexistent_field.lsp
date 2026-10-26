// Purpose: Accessing a struct field that does not exist is rejected.
struct point_t {
    int32_t x;
};
int32_t main() {
    point_t p = {1};
    return p.y;
}
