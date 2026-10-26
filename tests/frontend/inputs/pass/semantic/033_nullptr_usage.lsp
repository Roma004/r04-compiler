// Purpose: nullptr converts to pointers and compares with pointers.
int32_t main() {
    int32_t * p = nullptr;
    void_t * v = nullptr;
    if (p == nullptr) {
        p = nullptr;
    }
    if (nullptr == v) {
        return 0;
    }
    p = (int32_t *)nullptr;
    return (int32_t)nullptr == 0;
}
