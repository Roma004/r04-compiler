// Purpose: A struct is defined before functions that use it.
struct p_t {
    int32_t x;
};
int32_t make(int32_t v) {
    p_t p = [v];
    return p.x;
}
int32_t main() {
    return make(1);
}
