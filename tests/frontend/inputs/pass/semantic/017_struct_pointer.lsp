// Purpose: Field access through a struct pointer using -> and (*p).f.
struct point_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    point_t p = {1, 2};
    point_t * q = &p;
    q->x = 5;
    return (*q).y;
}
