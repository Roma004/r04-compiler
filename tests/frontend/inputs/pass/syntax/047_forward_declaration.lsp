// Purpose: A forward declaration enables a pointer variable before the body.
struct node_t;
node_t * g_head;
struct node_t {
    int32_t v;
    node_t * next;
};
int32_t main() {
    return 0;
}
