// Purpose: Globals without initializers are zero-initialized.
int32_t g;
uint8_t gu;
struct s_t {
    int32_t a;
    int32_t b;
};
s_t gs;
int32_t [3] ga;
int32_t main() {
    if (g != 0) {
        return 1;
    }
    return ga[0] + gs.a;
}
