// Purpose: Store and call a function pointer held in a struct field.
struct ops_t {
    int32_t(int32_t, int32_t) * add;
};
int32_t plus(int32_t a, int32_t b) {
    return a + b;
}
int32_t main() {
    ops_t o;
    o.add = &plus;
    return (*o.add)(1, 2);
}
