// Purpose: List initializers are not allowed in an assignment.
int32_t main() {
    int32_t [2] a;
    a = {1, 2};
    return 0;
}
