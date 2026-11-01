// Purpose: Casting an array to a pointer is ok.
int32_t main() {
    int32_t [3] a = [1, 2, 3];
    int32_t * p = (int32_t *)a;
    return 0;
}
