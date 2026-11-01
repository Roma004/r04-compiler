// Purpose: The struct keyword before a variable declaration is a syntax error.
struct p_t {
    int32_t x;
};
int32_t main() {
    struct p_t p;
    return 0;
}
