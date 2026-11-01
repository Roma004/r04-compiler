// Purpose: Every compound assignment operator on matching integer types.
int32_t main() {
    int32_t a = 10;
    int32_t b = 3;
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
