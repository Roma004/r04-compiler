// Purpose: Nested and empty blocks are valid statements.
int32_t main() {
    {
    }
    {
        {
        }
    }
    return 0;
}
