#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#include "bitset.hpp"
#include "macro_template.hpp"
#include "tools/iterator.hpp"

namespace tools {

/**
 * @brief An unordered set of elements stored in a single fixed-size block.
 *
 * Complexity:
 *  - insertion: O(1)
 *  - erase: O(1)
 *  - search: O(blk_size)
 *
 * @tparam T        Type of the stored elements.
 * @tparam blk_size Number of slots (must be 8, 16, 32 or 64).
 */
template <typename T, size_t blk_size = 64> class fixedset {
  public:
    using value_type = T;

  private:
    using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
    using block_t = std::array<storage_t, blk_size>;
    using map_t = bitset<blk_size>;

    using light_iterator = T *;

    struct iterator_content {
        using value_type = fixedset<T, blk_size>::value_type;

        bool operator==(const iterator_content &) const noexcept;
        value_type &get() noexcept;
        void next() noexcept;

        fixedset<T, blk_size> *parent;
        light_iterator it;
    };

  public:
    using iterator = ForwardIterator<iterator_content, false>;
    using const_iterator = ForwardIterator<iterator_content, true>;

    fixedset() noexcept;
    fixedset(const fixedset &);
    fixedset(fixedset &&) noexcept(HAS_NOEXCEPT_MOVE(T));
    ~fixedset();

    fixedset &operator=(const fixedset &);
    fixedset &operator=(fixedset &&) noexcept(
        HAS_NOEXCEPT_MOVE(T) && HAS_NOEXCEPT_SWAP(T)
    );

    /**
     * @brief Constructs an element in-place in the first free slot
     *
     * @tparam Args  Argument types forwarded to `T`'s constructor
     * @param  args  Arguments forwarded to `T`'s constructor
     * @return       Iterator to the newly inserted element
     * @throws std::length_error if the set is already full
     * @throws Any exception thrown by `T`'s constructor. In that case the
     *         set is left unchanged
     */
    template <typename_args_of(Args, T) = 0> iterator emplace(Args &&...args);

    /**
     * @brief Inserts a copy of an element into the first free slot
     *
     * @param val  Value to copy
     * @return     Iterator to the inserted element
     * @throws std::length_error if the set is already full
     */
    iterator insert(const T &);

    /**
     * @brief Move an element into the first free slot
     *
     * @param val  Value to move.
     * @return     Iterator to the inserted element
     * @throws std::length_error if the set is already full
     */
    iterator insert(T &&);

    /**
     * @brief Erases the element pointed to by the iterator
     *
     * Destroys the element and clears its slot. The passed iterator becomes
     * invalid; all other iterators remain valid
     *
     * @param it  Iterator to the element to erase
     * @return    Iterator to the next occupied slot, or `end()` if none
     * @throws std::runtime_error if `it` does not point to a live element
     *         of this set
     */
    iterator erase(iterator);

    void
    swap(fixedset &) noexcept(HAS_NOEXCEPT_MOVE(T) && HAS_NOEXCEPT_SWAP(T));

    /* @brief Returns true if `size()` == 0 */
    bool empty() const noexcept;

    /* @brief Returns true if every slot is occupied */
    bool full() const noexcept;

    /* @brief Returns the number of curerntly occupied slots */
    size_t size() const noexcept;

    /* @brief Returns the total number of slots */
    static constexpr size_t capacity() noexcept;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;

    /** @brief Returns an iterator to the slot at index `idx` */
    iterator iter_at(size_t idx) noexcept;
    const_iterator iter_at(size_t idx) const noexcept;

    bool is_valid(const_iterator) const noexcept;

    /** @brief Returns the slot index the iterator points to */
    size_t idx_of(const_iterator it) const noexcept;

  private:
    bool is_valid(light_iterator) const noexcept;
    light_iterator next_iterator(light_iterator) noexcept;
    size_t idx_of(light_iterator it) const noexcept;

    void destroy_all() noexcept;

    block_t block;
    map_t map{0};
};

#define FIXEDSET_TEMPLATE           template <typename T, size_t blk_size>
#define FIXEDSET_TEMPLATE_ARGS(...) FIXEDSET_TEMPLATE template <__VA_ARGS__>
#define FIXEDSET                    fixedset<T, blk_size>

// ---------------------------------------------------------------------------
// iterator implementation
// ---------------------------------------------------------------------------
//
FIXEDSET_TEMPLATE
bool FIXEDSET::iterator_content::operator==(
    const iterator_content &other
) const noexcept {
    return it == other.it;
}

FIXEDSET_TEMPLATE
void FIXEDSET::iterator_content::next() noexcept {
    assert(parent != nullptr);
    it = parent->next_iterator(it);
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator_content::value_type &
FIXEDSET::iterator_content::get() noexcept {
    return *it;
}

// ---------------------------------------------------------------------------
// fixedset implementation
// ---------------------------------------------------------------------------

FIXEDSET_TEMPLATE
FIXEDSET::fixedset() noexcept = default;

FIXEDSET_TEMPLATE
FIXEDSET::fixedset(const fixedset &o) {
    try {
        for (size_t i = 0; i < blk_size; ++i) {
            if (o.map.is_used(i)) {
                new (&block[i]) T(*o.iter_at(i));
                map.set(i);
            }
        }
    } catch (...) {
        destroy_all();
        map.clear();
        throw;
    }
}

FIXEDSET_TEMPLATE
FIXEDSET::fixedset(fixedset &&o) noexcept(HAS_NOEXCEPT_MOVE(T)) {
    for (size_t i = 0; i < blk_size; ++i) {
        if (o.map.is_used(i)) {
            new (&block[i]) T(std::move(*o.iter_at(i)));
            map.set(i);
        }
    }
    o.destroy_all();
    o.map.clear();
}

FIXEDSET_TEMPLATE
FIXEDSET::~fixedset() { destroy_all(); }

FIXEDSET_TEMPLATE
FIXEDSET &FIXEDSET::operator=(const fixedset &o) {
    if (this != &o) {
        fixedset tmp(o);
        swap(tmp);
    }
    return *this;
}

FIXEDSET_TEMPLATE
FIXEDSET &FIXEDSET::operator=(fixedset &&o) noexcept(
    HAS_NOEXCEPT_MOVE(T) && HAS_NOEXCEPT_SWAP(T)
) {
    if (this != &o) {
        fixedset tmp(std::move(o));
        swap(tmp);
    }
    return *this;
}

FIXEDSET_TEMPLATE
void FIXEDSET::swap(fixedset &o) noexcept(
    HAS_NOEXCEPT_MOVE(T) && HAS_NOEXCEPT_SWAP(T)
) {
    using std::swap;
    for (size_t i = 0; i < blk_size; ++i) {
        bool a_used = map.is_used(i);
        bool b_used = o.map.is_used(i);
        if (a_used && b_used) {
            swap(*iter_at(i), *o.iter_at(i));
        } else if (a_used) {
            new (&o.block[i]) T(std::move(*iter_at(i)));
            iter_at(i)->~T();
        } else if (b_used) {
            new (&block[i]) T(std::move(*o.iter_at(i)));
            o.iter_at(i)->~T();
        }
    }
    swap(map, o.map);
}

FIXEDSET_TEMPLATE_ARGS(typename_args_of(Args, T))
typename FIXEDSET::iterator FIXEDSET::emplace(Args &&...args) {
    ssize_t idx = map.find_free_idx();
    if (idx == -1) throw std::length_error("fixedset is full");
    new (&block[idx]) T(std::forward<Args&&>(args)...);
    map.set(idx);
    return iter_at(idx);
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::insert(const T &val) {
    return emplace(val);
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::insert(T &&val) {
    return emplace(std::move(val));
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::erase(iterator it) {
    if (!is_valid(it))
        throw std::runtime_error("iterator does not point to a live element");
    size_t idx = idx_of(it);
    it->~T();
    map.unset(idx);
    ssize_t next = map.find_used_idx();
    return next == -1 ? end() : iter_at(next);
}

FIXEDSET_TEMPLATE
bool FIXEDSET::is_valid(light_iterator it) const noexcept {
    const uint8_t *raw = (const uint8_t *)(void *)it;
    const uint8_t *data = (const uint8_t *)(void *)block.data();
    if (raw == nullptr) return false;
    if (raw < data || raw >= data + blk_size * sizeof(T)) return false;
    if ((raw - data) % sizeof(storage_t) != 0) return false;
    return map.is_used(idx_of(it));
}

FIXEDSET_TEMPLATE
bool FIXEDSET::is_valid(const_iterator it) const noexcept {
    return is_valid(it.get_content().it);
}

FIXEDSET_TEMPLATE
bool FIXEDSET::empty() const noexcept { return map.is_empty(); }

FIXEDSET_TEMPLATE
bool FIXEDSET::full() const noexcept { return map.is_full(); }

FIXEDSET_TEMPLATE
size_t FIXEDSET::size() const noexcept { return map.count(); }

FIXEDSET_TEMPLATE
constexpr size_t FIXEDSET::capacity() noexcept { return blk_size; }

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::begin() noexcept {
    ssize_t idx = map.find_used_idx();
    return idx == -1 ? end() : iter_at(idx);
}

FIXEDSET_TEMPLATE
typename FIXEDSET::const_iterator FIXEDSET::begin() const noexcept {
    ssize_t idx = map.find_used_idx();
    return idx == -1 ? end() : iter_at(idx);
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::end() noexcept {
    return iterator(iterator_content{this, nullptr});
}

FIXEDSET_TEMPLATE
typename FIXEDSET::const_iterator FIXEDSET::end() const noexcept {
    return const_iterator({const_cast<fixedset<T, blk_size> *>(this), nullptr});
}

FIXEDSET_TEMPLATE
typename FIXEDSET::light_iterator
FIXEDSET::next_iterator(light_iterator it) noexcept {
    if (!is_valid(it)) return end().get_content().it;
    size_t idx = idx_of(it);
    if (idx + 1 >= blk_size) return end().get_content().it;
    ssize_t next =
        bitset<blk_size>{
            (typename uint_t<blk_size>::type)(map.val >> (idx + 1))
        }
            .find_used_idx();
    if (next == -1) return end().get_content().it;
    return iter_at(idx + 1 + next).get_content().it;
}

FIXEDSET_TEMPLATE
typename FIXEDSET::iterator FIXEDSET::iter_at(size_t idx) noexcept {
    assert(idx < blk_size);
    return iterator(
        {this, std::launder(reinterpret_cast<light_iterator>(&block[idx]))}
    );
}

FIXEDSET_TEMPLATE
typename FIXEDSET::const_iterator FIXEDSET::iter_at(size_t idx) const noexcept {
    assert(idx < blk_size);
    block_t &blk = const_cast<block_t &>(block);
    return const_iterator(
        {const_cast<fixedset<T, blk_size> *>(this),
         std::launder(reinterpret_cast<light_iterator>(&blk[idx]))}
    );
}

FIXEDSET_TEMPLATE
size_t FIXEDSET::idx_of(const_iterator it) const noexcept {
    return reinterpret_cast<const storage_t *>(it.get_content().it)
         - block.data();
}

FIXEDSET_TEMPLATE
size_t FIXEDSET::idx_of(light_iterator it) const noexcept {
    return reinterpret_cast<const storage_t *>(it) - block.data();
}

FIXEDSET_TEMPLATE
void FIXEDSET::destroy_all() noexcept {
    for (size_t i = 0; i < blk_size; ++i)
        if (map.is_used(i)) iter_at(i)->~T();
}

FIXEDSET_TEMPLATE
inline void swap(FIXEDSET &a, FIXEDSET &b) noexcept(
    HAS_NOEXCEPT_MOVE(T) && HAS_NOEXCEPT_SWAP(T)
) {
    a.swap(b);
}

#undef FIXEDSET
#undef FIXEDSET_TEMPLATE
#undef FIXEDSET_TEMPLATE_ARGS

} // namespace tools
