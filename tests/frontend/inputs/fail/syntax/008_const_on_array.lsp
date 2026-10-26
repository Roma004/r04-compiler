// Purpose: const placed between an element type and an array modifier is a syntax error.
int32_t main() {
    int32_t const [3] a;
    return 0;
}
