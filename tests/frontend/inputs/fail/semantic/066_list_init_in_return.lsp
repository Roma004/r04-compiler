// Purpose: A list initializer is not allowed in a return statement.
int32_t [2] f() {
    return {1, 2};
}
int32_t main() {
    return 0;
}
