// Purpose: The unary operators -, ~, !, &, and *.
int32_t main() {
    int32_t x = 1;
    int32_t a = -x;
    int32_t b = ~x;
    int32_t c = !x;
    int32_t * p = &x;
    int32_t d = *p;
    return a + b + c + d;
}
