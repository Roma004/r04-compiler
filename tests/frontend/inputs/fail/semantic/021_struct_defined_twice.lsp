// Purpose: Defining the same struct twice is rejected.
struct s_t {
    int32_t a;
};
struct s_t {
    int32_t b;
};
int32_t main() {
    return 0;
}
