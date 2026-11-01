// Purpose: List initializers: nested, flat, partial, trailing comma, empty.
struct p_t {
    int32_t x;
    int32_t y;
};
int32_t main() {
    int32_t [3] a = [1, 2, 3,];
    int32_t [2][2] b = [[1, 2], [3, 4]];
    int32_t [2][2] c = [1, 2, 3, 4];
    int32_t [3] d = [];
    p_t e = [1, 2];
    p_t f = [1];
    return 0;
}
