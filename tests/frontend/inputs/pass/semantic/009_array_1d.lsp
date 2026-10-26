// Purpose: One-dimensional arrays with list initialization and subscripting.
int32_t main() {
    int32_t [3] a = {1, 2, 3};
    int32_t [3] b;
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
    return b[2];
}
