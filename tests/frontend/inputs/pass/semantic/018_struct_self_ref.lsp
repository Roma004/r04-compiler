// Purpose: A self-referential struct with pointer links and nullptr.
struct node_t;
struct node_t {
    int32_t value;
    node_t * next;
};
int32_t main() {
    node_t a;
    node_t b;
    a.next = &b;
    b.next = nullptr;
    return 0;
}
