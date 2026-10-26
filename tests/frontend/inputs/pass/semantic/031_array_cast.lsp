// Purpose: Array-to-array casts preserving total byte size.
int32_t main() {
    int32_t [2] a = {1, 2};
    int8_t [8] b = (int8_t [8])a;
    int32_t [2] c = (int32_t [2])b;
    return c[0];
}
