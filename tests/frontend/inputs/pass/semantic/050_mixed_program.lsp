// Purpose: A larger valid program combining structs, arrays, functions, and loops.
struct item_t {
    int32_t id;
    int32_t [2] values;
};
int32_t sum(item_t * it) {
    return it->id + it->values[0] + it->values[1];
}
int32_t main() {
    item_t items = {1, {2, 3}};
    int32_t total = 0;
    for (int32_t i = 0; i < 1; i += 1) {
        total += sum(&items);
    }
    return total;
}
