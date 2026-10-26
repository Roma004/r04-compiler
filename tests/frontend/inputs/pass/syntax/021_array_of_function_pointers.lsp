// Purpose: An array of function pointers.
int32_t add(int32_t a) {
    return a;
}
int32_t main() {
    int32_t(int32_t) *[3] fs = [&add, &add, &add, ];
    return 0;
}
