// Purpose: Two-dimensional arrays, nested and flat init, and whole-array copy.
int32_t main() {
    int32_t [2][3] a = {{1, 2, 3}, {4, 5, 6}};
    int32_t [2][3] b = {1, 2, 3, 4, 5, 6};
    int32_t [2][3] c;
    c = a;
    c[0][0] = b[1][2];
    return c[0][0];
}
