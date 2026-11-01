// Purpose: sizeof of an incomplete struct type is rejected.
struct s_t;
int32_t main() {
    uint64_t n = sizeof(s_t);
    return 0;
}
