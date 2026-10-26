// Purpose: Subscript, field access, arrow access, and function calls.
struct point_t {
    int32_t x;
    int32_t y;
};
int32_t f(int32_t a) {
    return a;
}
int32_t main() {
    int32_t [3] a = [1, 2, 3];
    point_t p = [1, 2];
    point_t * q = &p;
    int32_t i = a[0] + p.x + q->y + f(1);
    return i;
}
