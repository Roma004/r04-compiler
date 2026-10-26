// Purpose: Global variables with no initializer, scalar init, and list init.
int32_t g1;
int32_t g2 = 1;
int32_t [3] g3 = [1, 2, 3];
int32_t [2][2] g4 = [[1, 2], [3, 4]];
struct s_t {
    int32_t a;
    int32_t b;
};
s_t g5 = [1, 2];
int32_t main() {
    return 0;
}
