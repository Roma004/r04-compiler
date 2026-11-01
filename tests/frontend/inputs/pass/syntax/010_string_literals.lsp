// Purpose: String literals including escape sequences and the empty string.
int32_t main() {
    const int8_t *a = "hello";
    const int8_t *b = "line\n";
    const int8_t *c = "tab\tquote\"backslash\\";
    const int8_t *d = "";
    return 0;
}
