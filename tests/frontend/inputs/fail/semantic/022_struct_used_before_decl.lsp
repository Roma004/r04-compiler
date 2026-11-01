// Purpose: Using a struct before it is declared is rejected.
int32_t main() {
    s_t v;
    return 0;
}
struct s_t {
    int32_t a;
};
