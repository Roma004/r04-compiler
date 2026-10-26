// Purpose: An array argument decays to a pointer parameter.
int32_t first(int32_t * a) {
    return a[0];
}
int32_t main() {
    int32_t [3] a = {5, 6, 7};
    return first(a);
}
