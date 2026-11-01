// Purpose: An empty struct has size one and may be declared.
struct empty_t {
};
int32_t main() {
    empty_t e;
    uint64_t s = sizeof(empty_t);
    return 0;
}
