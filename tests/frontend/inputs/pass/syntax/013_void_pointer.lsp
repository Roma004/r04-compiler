// Purpose: void_t * in return and parameter position, and as a variable.

void_t *passthrough(void_t *src) {
    return src;
}

int32_t main() {
    void_t *p = nullptr;
    return 0;
}