// Purpose: Nested struct fields and nested list initialization.
struct inner_t {
    int32_t v;
};
struct outer_t {
    inner_t in;
    int32_t w;
};
int32_t main() {
    outer_t o = {{7}, 8};
    return o.in.v + o.w;
}
