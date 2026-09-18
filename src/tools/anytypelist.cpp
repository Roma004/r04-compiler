#include "tools/anytypelist.hpp"
#include <cassert>

namespace tools {

bool anytypelist::iterator_content::operator==(
    const iterator_content &other
) const noexcept {
    return ptr == other.ptr;
}

anytypelist::iterator_content::value_type &
anytypelist::iterator_content::get() noexcept {
    return *ptr;
}

void anytypelist::iterator_content::next() noexcept { ptr = ptr->next.get(); }

// ---------------------------------------------------------------------------
// anytypelist
// ---------------------------------------------------------------------------

anytypelist::anytypelist(anytypelist &&o) noexcept :
    head(std::move(o.head)), elements_num(o.elements_num) {
    o.elements_num = 0;
}

anytypelist &anytypelist::operator=(anytypelist &&o) noexcept {
    if (this != &o) {
        clear();
        head = std::move(o.head);
        elements_num = o.elements_num;
        o.elements_num = 0;
    }
    return *this;
}

anytypelist::~anytypelist() { clear(); }

anytypelist::iterator anytypelist::begin() noexcept {
    return iterator({head.get()});
}

anytypelist::iterator anytypelist::end() noexcept {
    return iterator({nullptr});
}

anytypelist::const_iterator anytypelist::begin() const noexcept {
    return const_iterator({head.get()});
}

anytypelist::const_iterator anytypelist::end() const noexcept {
    return const_iterator({nullptr});
}

bool anytypelist::empty() const noexcept { return head == nullptr; }

size_t anytypelist::size() const noexcept { return elements_num; }

anytypelist::iterator anytypelist::erase(iterator pos) noexcept {
    assert(pos != end());

    base_node_t *n = &*pos;
    base_node_t *prev = n->prev;
    base_node_t *next = n->next.get();

    assert(n != nullptr);

    if (pos == begin()) {
        auto moved = std::move(head);
        head = std::move(moved->next);
        if (head != nullptr) head->prev = nullptr;
    } else {
        assert(prev != nullptr); // if is not a head, prev != nullptr

        auto moved = std::move(prev->next);
        prev->next = std::move(moved->next);
        if (prev->next != nullptr) prev->next->prev = prev;
    }

    --elements_num;
    return iterator({next});
}

void anytypelist::clear() noexcept {
    while (head) {
        auto *raw = head->next.release();
        head.reset(raw);
    }
    elements_num = 0;
}

} // namespace tools
