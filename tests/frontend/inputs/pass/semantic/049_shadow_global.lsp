// Purpose: Local variables may shadow a global of the same name.
int32_t value = 1;
int32_t main() {
    int32_t value = 2;
    {
        int32_t value = 3;
        value = value + 1;
    }
    return value;
}
