// Purpose: An array of pointers with an address-of initializer list.
int32_t main() {
    int32_t x = 1;
    int32_t y = 2;
    int32_t *[2] arr = {&x, &y};
    return *arr[0];
}
