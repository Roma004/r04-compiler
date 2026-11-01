// Purpose: Inner scopes may shadow names from enclosing scopes.
int32_t main() {
    int32_t x = 1;
    {
        int32_t x = 2;
        {
            int32_t x = 3;
            x = x + 1;
        }
    }
    return x;
}
