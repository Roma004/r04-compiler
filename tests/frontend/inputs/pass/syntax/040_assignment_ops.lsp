// Purpose: Simple assignment and every compound assignment operator.
int32_t main() {
    int32_t a = 10;
    int32_t b = 2;
    a = b;
    a += b;
    a -= b;
    a *= b;
    a /= b;
    a %= b;
    a &= b;
    a |= b;
    a ^= b;
    a <<= b;
    a >>= b;
    return a;
}
