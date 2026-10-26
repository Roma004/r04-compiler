// Purpose: String literals have type const int8_t * const and are readable.
int32_t main() {
    const int8_t * const msg = "line\n\t\"\\";
    int8_t c = msg[0];
    return 0;
}
