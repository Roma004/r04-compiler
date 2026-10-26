// Purpose: Struct definitions are only allowed at global scope.
int32_t main() {
    struct s_t {
        int32_t a;
    };
    return 0;
}
