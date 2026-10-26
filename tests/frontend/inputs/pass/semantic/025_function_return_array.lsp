// Purpose: A function may return an array, copied element by element.
int32_t [3] make() {
    int32_t [3] a = {1, 2, 3};
    return a;
}
int32_t main() {
    int32_t [3] a = make();
    return a[0];
}
