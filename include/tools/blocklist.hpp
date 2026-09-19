#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <sys/types.h>
#include <utility>

#include "tools/fixedset.hpp"
#include "tools/iterator.hpp"
#include "tools/macro_template.hpp"

namespace tools {

/**
 * @brief A container that stores elements in a list of fixed-size blocks.
 *
 * Complexity:
 *  - insert: O(N / blk_size)
 *  - search: linear
 *  - erase: O(1)
 *
 * Operations:
 * - Move and Insertion does not affect any existing iterators
 * - Erasure invalidates only iterators to the element erased
 * - Copy is not implemented
 *
 * @tparam T        Type of the stored elements
 * @tparam blk_size Number of elements per block (must be 8, 16, 32 or 64)
 */
template <typename T, size_t blk_size = 64> class blocklist {
  protected:
    using block_t = fixedset<T, blk_size>;
    using list_t = std::list<block_t>;

    struct iterator_content {
        using value_type = T;
        using list_iterator = typename list_t::iterator;

        bool operator==(const iterator_content &) const noexcept;
        value_type &get() noexcept;
        void next() noexcept;

        size_t get_idx();

        list_iterator lst_it;
        size_t blk_idx;
    };

  public:
    using value_type = T;

    /** @brief Forward iterator over the elements of the container */
    using iterator = ForwardIterator<iterator_content, false>;
    using const_iterator = ForwardIterator<iterator_content, true>;

    /**
     * @brief Constructs an element in-place and inserts it
     *
     * @tparam Args  Argument types forwarded to T's constructor
     * @param  args  Arguments forwarded to T's constructor
     * @return       Iterator to the newly inserted element
     */
    template <typename_args_of(Args, T) = 0> iterator emplace(Args &&...args);

    /**
     * @brief Inserts a copy of an element.
     *
     * @param val  Value to copy
     * @return     Iterator to the inserted element
     */
    iterator insert(const T &);

    /**
     * @brief Inserts an element by moving it
     *
     * @param val  Value to move
     * @return     Iterator to the inserted element
     */
    iterator insert(T &&);

    /**
     * @brief Erases the element pointed to by the iterator.
     *
     * The iterator becomes invalid. All other iterators remain valid.
     *
     * @param it  Iterator to the element to erase.
     * @throws std::runtime_error if the iterator was already invalid.
     */
    iterator erase(iterator);

    /** @brief returns true is size() == 0 */
    constexpr bool empty() const noexcept;

    /** @brief returns number of elements in container */
    constexpr size_t size() const noexcept;

    /** @brief returns current amount af allocated memory in elements */
    constexpr size_t capacity() const noexcept;

    void clear() noexcept;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;

  private:
    using list_iterator = typename list_t::iterator;
    using block_iterator = typename block_t::iterator;
    using const_list_iterator = typename list_t::const_iterator;
    using const_block_iterator = typename block_t::const_iterator;

    iterator find_empty() noexcept;
    void erase_block(typename list_t::iterator);
    iterator make_block();

    iterator make_iter(list_iterator, block_iterator) noexcept;
    iterator make_iter(list_iterator) noexcept;

    list_t blocks;
    size_t elements_num = 0;
};

static_assert(sizeof(blocklist<int>::iterator) == 16);
static_assert(sizeof(blocklist<int>::const_iterator) == 16);

#define BLOCKLIST_TEMPLATE           template <typename T, size_t blk_size>
#define BLOCKLIST_TEMPLATE_ARGS(...) BLOCKLIST_TEMPLATE template <__VA_ARGS__>
#define BLOCKLIST                    blocklist<T, blk_size>

// ---------------------------------------------------------------------------
// iterator implementation
// ---------------------------------------------------------------------------

BLOCKLIST_TEMPLATE
bool BLOCKLIST::iterator_content::operator==(
    const iterator_content &other
) const noexcept {
    return lst_it == other.lst_it
        && const_cast<iterator_content &>(*this).get_idx()
               == const_cast<iterator_content &>(other).get_idx();
}

BLOCKLIST_TEMPLATE
void BLOCKLIST::iterator_content::next() noexcept {
    auto blk_next = std::next(lst_it->iter_at(get_idx()));
    if (blk_next != lst_it->end()) {
        blk_idx = lst_it->idx_of(blk_next);
        return;
    }
    ++lst_it;
    blk_idx = (size_t)-1;
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator_content::value_type &
BLOCKLIST::iterator_content::get() noexcept {
    return *(lst_it->iter_at(get_idx()));
}

BLOCKLIST_TEMPLATE
size_t BLOCKLIST::iterator_content::get_idx() {
    if (blk_idx == (size_t)-1) blk_idx = lst_it->idx_of(lst_it->begin());
    return blk_idx;
}

// ---------------------------------------------------------------------------
// blocklist implementation
// ---------------------------------------------------------------------------

BLOCKLIST_TEMPLATE_ARGS(typename_args_of(Args, T))
typename BLOCKLIST::iterator BLOCKLIST::emplace(Args &&...args) {
    auto lst_it = blocks.begin();
    for (; lst_it != blocks.end(); ++lst_it) {
        if (!lst_it->full()) break;
    }
    if (lst_it == blocks.end()) {
        blocks.emplace_back();
        lst_it = std::prev(blocks.end());
    }

    auto blk_it = lst_it->emplace(std::forward<Args &&>(args)...);
    elements_num += 1;
    return make_iter(lst_it, blk_it);
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator BLOCKLIST::insert(const T &val) {
    return emplace(val);
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator BLOCKLIST::insert(T &&val) {
    return emplace(std::move(val));
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator BLOCKLIST::erase(iterator it) {
    iterator next = std::next(it);
    auto &lst = *it.get_content().lst_it;
    lst.erase(lst.iter_at(it.get_content().get_idx()));
    elements_num -= 1;
    if (lst.empty()) blocks.erase(it.get_content().lst_it);
    return next;
}

BLOCKLIST_TEMPLATE
constexpr bool BLOCKLIST::empty() const noexcept {
    // return empty list if only sentinel node exists
    return blocks.size() == 0;
}

BLOCKLIST_TEMPLATE
constexpr size_t BLOCKLIST::size() const noexcept { return elements_num; }

BLOCKLIST_TEMPLATE
constexpr size_t BLOCKLIST::capacity() const noexcept {
    return blocks.size() * blk_size;
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator BLOCKLIST::begin() noexcept {
    if (empty()) return end();
    return make_iter(blocks.begin());
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::const_iterator BLOCKLIST::begin() const noexcept {
    if (empty()) return end();
    BLOCKLIST *self = const_cast<BLOCKLIST *>(this);
    return self->make_iter(self->blocks.begin());
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator BLOCKLIST::end() noexcept {
    return iterator({blocks.end(), (size_t)-1});
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::const_iterator BLOCKLIST::end() const noexcept {
    BLOCKLIST *self = const_cast<BLOCKLIST *>(this);
    return const_iterator({self->blocks.end(), (size_t)-1});
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator
BLOCKLIST::make_iter(list_iterator lst_it, block_iterator blk_it) noexcept {
    size_t idx = lst_it->idx_of(blk_it);
    return iterator({lst_it, idx});
}

BLOCKLIST_TEMPLATE
typename BLOCKLIST::iterator
BLOCKLIST::make_iter(list_iterator lst_it) noexcept {
    return make_iter(lst_it, lst_it->begin());
}

BLOCKLIST_TEMPLATE
void BLOCKLIST::clear() noexcept { blocks.clear(); }

#undef BLOCKLIST
#undef BLOCKLIST_TEMPLATE
#undef BLOCKLIST_TEMPLATE_ARGS

} // namespace tools
