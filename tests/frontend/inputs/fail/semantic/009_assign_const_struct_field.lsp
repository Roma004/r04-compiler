// Purpose: Assigning through a pointer to a const struct is rejected.
struct item_t {
    int32_t x;
};
int32_t main() {
    item_t v = {1};
    const item_t * p = &v;
    p->x = 2;
    return 0;
}
