// Purpose: Every binary operator appears in a valid expression.
int32_t main() {
    int32_t a = 6;
    int32_t b = 3;
    int32_t c = a * b / 2 % 4;
    int32_t d = a + b - 1;
    int32_t e = a << 1 >> 1;
    int32_t f = (a < b) + (a > b) + (a <= b) + (a >= b);
    int32_t g = (a == b) + (a != b);
    int32_t h = a & b | a ^ b;
    int32_t i = (a && b) || (a || b);
    return a;
}
