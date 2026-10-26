// Purpose: A pointer to an array, obtained with address-of.
int32_t main() {
    int32_t [3] a = {1, 2, 3};
    int32_t [3] * p = &a;
    return (*p)[1];
}
