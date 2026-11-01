// Purpose: Two-dimensional arrays, nested and flat init, and whole-array copy.
int32_t main() {
    int32_t [2][3] a = [[1, 2], [3, 4], [5, 6]];
    int32_t [2][3] b;
    b = a;
    b[2][1] = a[2][1];
    return b[0][0];
}
