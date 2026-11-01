// Purpose: Identifiers may contain underscores; names ending in _t are types.
struct my_struct_t {
    int32_t _field1;
};
int32_t _helper(int32_t __x) {
    return __x;
}
int32_t main() {
    my_struct_t v;
    int32_t __y = _helper(1);
    return __y;
}
