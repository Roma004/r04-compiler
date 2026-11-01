// Purpose: Subscripting a value that is not an array or pointer is rejected.
int32_t main() {
    int32_t x = 1;
    return x[0];
}
