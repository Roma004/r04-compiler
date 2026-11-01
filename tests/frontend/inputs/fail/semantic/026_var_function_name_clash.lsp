// Purpose: A global variable and a function may not share a name.
int32_t g = 1;
int32_t g() {
    return 0;
}
int32_t main() {
    return 0;
}
