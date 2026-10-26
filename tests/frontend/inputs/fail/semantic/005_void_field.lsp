// Purpose: void_t may not be used as a struct field type.
struct bad_t {
    void_t v;
};
int32_t main() {
    return 0;
}
