// Purpose: A struct whose field has another struct type.
struct inner_t {
    int32_t v;
};
struct outer_t {
    inner_t in;
    int32_t w;
};
int32_t main() {
    outer_t o;
    return o.in.v;
}
