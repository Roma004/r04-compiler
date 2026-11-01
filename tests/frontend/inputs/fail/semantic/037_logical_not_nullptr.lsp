// Purpose: The logical not operator requires an integer operand, not nullptr.
int32_t main() {
    int32_t x = !nullptr;
    return x;
}
