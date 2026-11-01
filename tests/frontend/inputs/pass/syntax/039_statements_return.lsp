// Purpose: return with and without an expression.
void_t nothing() {
    return;
}
int32_t one() {
    return 1;
}
int32_t main() {
    nothing();
    return one();
}
