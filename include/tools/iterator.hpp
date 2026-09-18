#pragma once

#include "macro_template.hpp"
#include <iterator>
#include <type_traits>
#include <utility>

namespace tools {

#define ITERATOR_TEMPLATE       template <typename content_type, bool is_const>
#define ITERATOR_OTHER_TEMPLATE template <bool other_const>
#define ITERATOR_OTHER_TEMPLATE_COND(cond) \
    template <bool other_const, std::enable_if_t<cond, int> = 0>

#define ITERATOR       ForwardIterator<content_type, is_const>
#define ITERATOR_OTHER ForwardIterator<content_type, other_const>

/**
 * @brief A generic forward iterator adapter parameterized by a position type.
 *
 * `ForwardIterator` does not know anything about the container it walks. All
 * the container-specific logic is delegated to `content_type`, which must
 * provide:
 *  - `value_type`        -- type of element returned by *it
 *  - `void next()`       -- method that advances the position
 *  - `value_type &get()` -- method that returns the current element
 *  - `bool operator==(const content_type &) const`
 *
 * @tparam content_type  Position type. Must satisfy the four requirements
 *                       listed above; they are enforced by `static_assert`
 * @tparam is_const      `true` to produce a const iterator, `false` for a
 *                       mutable one
 *
 * @par Example
 * @code
 *   class MyContainer {
 *       struct iterator_content {
 *           using value_type = int;
 *           int &get() noexcept { return *ptr; }
 *           void next() noexcept { ++ptr; }
 *           bool operator==(const iterator_content &o) const noexcept {
 *               return ptr == o.ptr;
 *           }
 *           int *ptr;
 *       };
 *     public:
 *       using iterator       = ForwardIterator<iterator_content, false>;
 *       using const_iterator = ForwardIterator<iterator_content, true>;
 *   };
 * @endcode
 *
 * @note Do not provide a `const` overload of `get()`; the iterator only ever
 *       calls the non-const one
 */
ITERATOR_TEMPLATE class ForwardIterator {
    /** @brief Compile-time requirements imposed on `content_type` */
    struct req {
        template <typename T>
        using next_op =
            detail::ret_type_is<decltype(std::declval<T>().next()), void>;

        template <typename T>
        using get_op = detail::ret_type_is<
            decltype(std::declval<T>().get()),
            typename T::value_type &>;

        template <typename T> using value_type_op = typename T::value_type;
    };
    static_assert(
        detail::has_attr_v<req::template value_type_op, content_type>,
        "type content_type::value_type must be defined"
    );
    static_assert(
        detail::has_attr_v<req::template next_op, content_type>,
        "method content_type::next must be defined"
    );
    static_assert(
        detail::has_attr_v<req::template get_op, content_type>,
        "method content_type::get must be defined"
    );
    static_assert(
        detail::has_attr_v<detail::compare_operator, content_type>,
        "method content_type::operator== must be defined"
    );

    friend class ForwardIterator<content_type, !is_const>;

  public:
    using value_type = typename content_type::value_type;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using pointer =
        std::conditional_t<is_const, const value_type, value_type> *;
    using reference =
        std::conditional_t<is_const, const value_type, value_type> &;

    /**
     * @brief Constructs a singular (end-like) iterator
     *
     * The default-constructed iterator compares equal to another
     * default-constructed one and is not dereferenceable
     */
    ForwardIterator() noexcept = default;

    /** @brief Constructs an iterator from a position object */
    ForwardIterator(content_type &&) noexcept(HAS_NOEXCEPT_MOVE(content_type));

    /**
     * @brief Converting copy constructor from a (possibly) more mutable
     *        iterator.
     */
    ITERATOR_OTHER_TEMPLATE_COND(
        is_const >= other_const && std::is_copy_constructible_v<content_type>
    )
    ForwardIterator(const ITERATOR_OTHER &);

    /**
     * @brief Converting move constructor from a (possibly) more mutable
     *        iterator.
     */
    ITERATOR_OTHER_TEMPLATE_COND(
        is_const >= other_const && std::is_move_constructible_v<content_type>
    )
    ForwardIterator(ITERATOR_OTHER &&);

    /**
     * @brief Converting copy assignment from a (possibly) more mutable
     *        iterator.
     */
    ITERATOR_OTHER_TEMPLATE_COND(
        is_const >= other_const && std::is_copy_assignable_v<content_type>
    )
    ITERATOR &
    operator=(const ITERATOR_OTHER &) noexcept(HAS_NOEXCEPT_COPY(content_type));

    /**
     * @brief Converting move assignment from a (possibly) more mutable
     *        iterator.
     */
    ITERATOR_OTHER_TEMPLATE_COND(
        is_const >= other_const && std::is_move_assignable_v<content_type>
    )
    ITERATOR &
    operator=(ITERATOR_OTHER &&) noexcept(HAS_NOEXCEPT_MOVE(content_type));

    ITERATOR_OTHER_TEMPLATE
    bool operator==(const ITERATOR_OTHER &) const noexcept;

    ITERATOR_OTHER_TEMPLATE
    bool operator!=(const ITERATOR_OTHER &) const noexcept;

    ITERATOR &operator++() noexcept;
    ITERATOR operator++(int) noexcept;

    reference operator*() const noexcept;
    pointer operator->() const noexcept;

    template <bool B = is_const, std::enable_if_t<!B>>
    reference operator*() noexcept;

    template <bool B = is_const, std::enable_if_t<!B>>
    pointer operator->() noexcept;

    content_type &get_content() noexcept;
    const content_type &get_content() const noexcept;

  private:
    content_type ct;
};

#undef ITERATOR_OTHER_TEMPLATE_COND
#define ITERATOR_OTHER_TEMPLATE_COND(cond) \
    template <bool other_const, std::enable_if_t<cond, int>>

ITERATOR_TEMPLATE
ITERATOR::ForwardIterator(content_type &&ct) noexcept(
    HAS_NOEXCEPT_MOVE(content_type)
) : ct(std::move(ct)) {}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE_COND(
    is_const >= other_const && std::is_copy_constructible_v<content_type>
)
ITERATOR::ForwardIterator(const ITERATOR_OTHER &other) : ct(other.ct) {}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE_COND(
    is_const >= other_const && std::is_move_constructible_v<content_type>
)
ITERATOR::ForwardIterator(ITERATOR_OTHER &&other) : ct(other.ct) {}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE_COND(
    is_const >= other_const && std::is_copy_assignable_v<content_type>
)
ITERATOR &ITERATOR::operator=(const ITERATOR_OTHER &other) noexcept(
    HAS_NOEXCEPT_COPY(content_type)
) {
    ct = other.ct;
    return *this;
}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE_COND(
    is_const >= other_const && std::is_move_assignable_v<content_type>
)
ITERATOR &ITERATOR::operator=(ITERATOR_OTHER &&other) noexcept(
    HAS_NOEXCEPT_MOVE(content_type)
) {
    ct = std::move(other.ct);
    return *this;
}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE
bool ITERATOR::operator==(const ITERATOR_OTHER &other) const noexcept {
    return ct == other.ct;
}

ITERATOR_TEMPLATE
ITERATOR_OTHER_TEMPLATE
bool ITERATOR::operator!=(const ITERATOR_OTHER &other) const noexcept {
    return !(*this == other);
}

ITERATOR_TEMPLATE
ITERATOR &ITERATOR::operator++() noexcept {
    ct.next();
    return *this;
}

ITERATOR_TEMPLATE
ITERATOR ITERATOR::operator++(int) noexcept {
    ITERATOR tmp = *this;
    ++(*this);
    return tmp;
}

ITERATOR_TEMPLATE
typename ITERATOR::reference ITERATOR::operator*() const noexcept {
    return const_cast<content_type &>(ct).get();
}

ITERATOR_TEMPLATE
typename ITERATOR::pointer ITERATOR::operator->() const noexcept {
    return &const_cast<content_type &>(ct).get();
}

ITERATOR_TEMPLATE
template <bool B, std::enable_if_t<!B>>
typename ITERATOR::reference ITERATOR::operator*() noexcept {
    return ct.get();
}

ITERATOR_TEMPLATE
template <bool B, std::enable_if_t<!B>>
typename ITERATOR::pointer ITERATOR::operator->() noexcept {
    return &(ct.get());
}

ITERATOR_TEMPLATE
content_type &ITERATOR::get_content() noexcept { return ct; }

ITERATOR_TEMPLATE
const content_type &ITERATOR::get_content() const noexcept { return ct; }

#undef ITERATOR_TEMPLATE
#undef ITERATOR_OTHER_TEMPLATE
#undef ITERATOR_OTHER_TEMPLATE_COND
#undef ITERATOR
#undef ITERATOR_OTHER

} // namespace tools
