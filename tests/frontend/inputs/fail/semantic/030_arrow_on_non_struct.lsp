// Purpose: Field access with -> requires a pointer to a struct.
int32_t main() {
    int32_t x = 1;
    int32_t * p = &x;
    return p->y;
}
