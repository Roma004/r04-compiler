// Purpose: Unparenthesized operator chains exercise precedence and associativity.
int32_t main() {
    int32_t a = 1;
    int32_t b = 2;
    int32_t c = 3;
    int32_t d = a + b * c - a / b % c;
    int32_t e = a << b + c;
    int32_t f = a & b | c ^ a;
    int32_t g = a < b && b < c || a == c;
    int32_t h = !a + ~b - -c;
    return d;
}
