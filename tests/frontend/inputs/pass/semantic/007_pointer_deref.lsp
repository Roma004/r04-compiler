// Purpose: Address-of, dereference, and pointer-to-pointer writes.
int32_t main() {
    int32_t x = 7;
    int32_t * p = &x;
    *p = 9;
    int32_t ** pp = &p;
    **pp = 11;
    return x;
}
