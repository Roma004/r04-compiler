// Purpose: if and if-else with and without braces, including nesting.
int32_t main() {
    int32_t x = 1;
    if (x) {
        x = 2;
    }
    if (x) x = 3; else x = 4;
    if (x) {
        if (x) {
            x = 5;
        } else {
            x = 6;
        }
    } else {
        x = 7;
    }
    return x;
}
