// Purpose: Comparison, logical, bitwise, and unary operators.
int32_t main() {
    int32_t a = 1;
    int32_t b = 2;
    int32_t c = a < b;
    int32_t d = a == b;
    int32_t e = a != b;
    int32_t f = a <= b;
    int32_t g = a >= b;
    int32_t h = a > b;
    int32_t i = a && b;
    int32_t j = a || b;
    int32_t k = !a;
    int32_t l = a & b;
    int32_t m = a | b;
    int32_t n = a ^ b;
    int32_t o = ~a;
    int32_t p = -a;
    return c + d + e + f + g + h + i + j + k + l + m + n + o + p;
}
