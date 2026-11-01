// Purpose: Character literals, including escapes, have type int8_t.
int32_t main() {
    int8_t a = 'a';
    int8_t b = '\n';
    int8_t c = '\t';
    int8_t d = '\\';
    int8_t e = '\'';
    int8_t f = '\0';
    int8_t g = '\x41';
    int8_t h = '\101';
    return 0;
}
