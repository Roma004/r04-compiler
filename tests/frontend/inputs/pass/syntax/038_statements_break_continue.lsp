// Purpose: break and continue inside a for loop body.
int32_t main() {
    for (int32_t i = 0; i < 10; i += 1) {
        if (i == 2) continue;
        if (i == 8) break;
    }
    return 0;
}
