// Purpose: for loops with, without, and with multiple init/step elements.
int32_t main() {
    int32_t sum = 0;
    for (int32_t i = 0; i < 10; i += 1) {
        sum += i;
    }
    for (int32_t i = 0, int32_t j = 10; i < j; i += 1, j -= 1) {
        sum += i;
    }
    for (;;) {
        break;
    }
    for (int32_t i = 0; ; i += 1) {
        if (i > 3) break;
    }
    for (; sum > 100; ) {
        sum -= 1;
    }
    return sum;
}
