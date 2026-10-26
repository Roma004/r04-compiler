// Purpose: Defining the same function twice is rejected.
int32_t f() {
    return 1;
}
int32_t f() {
    return 2;
}
int32_t main() {
    return 0;
}
