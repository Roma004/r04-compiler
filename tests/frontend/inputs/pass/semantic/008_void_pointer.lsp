// Purpose: void_t * variables, parameters, returns, and pointer compatibility.
void_t * identity(void_t * p) {
    return p;
}
int32_t main() {
    int32_t x = 3;
    int32_t * p = &x;
    void_t * v = p;
    int32_t * q = v;
    void_t * w = identity(v);
    return *q;
}
