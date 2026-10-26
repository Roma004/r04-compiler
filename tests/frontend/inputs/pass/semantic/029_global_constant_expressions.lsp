// Purpose: Global initializers built from constant expressions and addresses.
int32_t g = 3;
int32_t h = 2 * 3 + 1;
uint64_t sz = sizeof(int32_t);
int32_t * gp = &g;
int32_t [4] arr = {1, 2, 3, 4};
int32_t * ep = &arr[2];
int32_t ident(int32_t x) {
    return x;
}
int32_t(int32_t) * fp = &ident;
int32_t main() {
    return h;
}
