// Purpose: nullptr is not an integer and cannot be an if condition.
int32_t main() {
    if (nullptr) {
        return 1;
    }
    return 0;
}
