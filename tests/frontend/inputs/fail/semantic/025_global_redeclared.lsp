// Purpose: Redeclaring a global variable is rejected.
int32_t g = 1;
int32_t g = 2;
int32_t main() {
    return 0;
}
