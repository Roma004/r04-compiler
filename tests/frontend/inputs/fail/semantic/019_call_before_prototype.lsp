// Purpose: Calling a function before its declaration is rejected.
int32_t main() {
    return helper();
}
int32_t helper() {
    return 1;
}
