// Purpose: A struct field whose type is a function pointer.
struct handler_t {
    int32_t(int32_t) * fn;
};
int32_t main() {
    return 0;
}
