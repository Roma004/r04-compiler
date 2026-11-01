// Purpose: A pointer to a function pointer can be dereferenced twice to call.
int32_t inc(int32_t x) {
    return x + 1;
}
int32_t main() {
    int32_t(int32_t) * fp = &inc;
    int32_t(int32_t) ** pp = &fp;
    return (*(*pp))(1);
}
