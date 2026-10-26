// Purpose: A recursive function is visible through its own prototype.
int32_t fact(int32_t n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}
int32_t main() {
    return fact(5);
}
