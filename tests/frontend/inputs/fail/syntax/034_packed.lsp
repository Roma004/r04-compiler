// Purpose: A struct definition followed by an extra identifier is a syntax error.
struct p_t {
    int32_t x;
} packed;
int32_t main() {
    return 0;
}
