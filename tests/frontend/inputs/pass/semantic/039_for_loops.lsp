// Purpose: for loops with declarations, multiple init/step elements, break, continue.
int32_t main() {
    int32_t sum = 0;
    for (int32_t i = 0; i < 5; i += 1) {
        if (i == 2) {
            continue;
        }
        if (i == 4) {
            break;
        }
        sum += i;
    }
    for (;;) {
        break;
    }
    for (int32_t a = 0, int32_t b = 1; a < 3; a += 1, b += 1) {
        sum += b;
    }
    return sum;
}
