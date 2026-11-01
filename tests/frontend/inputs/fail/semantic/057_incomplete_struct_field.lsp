// Purpose: A struct field may not have an incomplete type by value.
struct a_t;
struct b_t {
    a_t field;
};
int32_t main() {
    return 0;
}
