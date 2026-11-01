// Purpose: An else clause without a matching if is a syntax error.
int32_t main() {
    else {
        return 1;
    }
    return 0;
}
