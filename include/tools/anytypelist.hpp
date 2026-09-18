#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#include "tools/iterator.hpp"

namespace tools {

/**
 * @brief Heterogeneous double-linked list
 *
 * Operations:
 * - `emplace_front` / `push_front` do **not** invalidate existing iterators
 * - `erase` invalidates the erased iterator
 * - `clear`, move-assignment and destruction invalidate all iterators
 *
 * @par Example
 * @code
 * tools::anytypelist l;
 * l.emplace_front<int>(42);
 * l.emplace_front<std::string>("hello");
 *
 * for (auto it = l.begin(); it != l.end();) {
 *     if (it->is<std::string>())
 *         std::cout << it->get<std::string>() << std::endl;
 *     if (it->is<int>)
 *         it = l.erase(it);
 *     else
 *         ++it;
 * }
 * @endcode
 */
class anytypelist {
    struct base_node_t {
        virtual ~base_node_t() = default;

        template <typename T> bool is() const noexcept;
        template <typename T> T &get();
        template <typename T> const T &get() const;

      private:
        friend class anytypelist;

        std::unique_ptr<base_node_t> next;
        base_node_t *prev;
    };

    template <typename T> struct node_t final : base_node_t {
        T value;

        template <typename_args_of(Args, T) = 0>
        explicit node_t(Args &&...args);
    };

    struct iterator_content {
        using value_type = base_node_t;

        bool operator==(const iterator_content &) const noexcept;
        value_type &get() noexcept;
        void next() noexcept;

        base_node_t *ptr = nullptr;
    };

  public:
    using iterator = ForwardIterator<iterator_content, false>;
    using const_iterator = ForwardIterator<iterator_content, true>;

    anytypelist() = default;
    anytypelist(const anytypelist &) = delete;
    anytypelist(anytypelist &&o) noexcept;

    anytypelist &operator=(const anytypelist &o) = delete;
    anytypelist &operator=(anytypelist &&o) noexcept;

    ~anytypelist();

    iterator begin() noexcept;
    iterator end() noexcept;

    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    bool empty() const noexcept;
    size_t size() const noexcept;

    /**
     * @brief Constructs a new node of type @p T at the front of the list
     *
     * @tparam T Stored value type. Must derive from @ref base_node_t
     * @tparam Args Forwarded argument types
     * @param args Arguments forwarded to `T`'s constructor
     * @return Iterator to the newly stored value
     */
    template <typename T, typename_args_of(Args, T) = 0>
    iterator emplace_front(Args &&...args);

    /**
     * @brief Constructs a new node of type @p T from decayed @p value
     *
     * @tparam T Deduced value type
     * @param value Value to store
     * @return Iterator to the newly stored value
     */
    template <typename T> iterator push_front(T &&value);

    /**
     * @brief Removes the node pointed to by @p pos. Invalidates @p pos
     *
     * @param pos Iterator to the node to remove. Must belong to `*this` and
     *            must not be @ref end()
     * @return Iterator to the following element, or @ref end()
     */
    iterator erase(iterator pos) noexcept;

    void clear() noexcept;

  private:
    std::unique_ptr<base_node_t> head;
    size_t elements_num = 0;
};

// ---------------------------------------------------------------------------
// node_t
// ---------------------------------------------------------------------------

template <typename T>
template <typename_args_of(Args, T)>
anytypelist::node_t<T>::node_t(Args &&...args) :
    value(std::forward<Args>(args)...) {}

template <typename T> bool anytypelist::base_node_t::is() const noexcept {
    return typeid(node_t<T>) == typeid(*this);
}

template <typename T> T &anytypelist::base_node_t::get() {
    return dynamic_cast<node_t<T> *>(this)->value;
}

template <typename T> const T &anytypelist::base_node_t::get() const {
    return dynamic_cast<node_t<T> *>(const_cast<base_node_t *>(this))->value;
}

template <typename T, typename_args_of(Args, T)>
anytypelist::iterator anytypelist::emplace_front(Args &&...args) {
    auto node = std::make_unique<node_t<T>>(std::forward<Args>(args)...);
    base_node_t *raw = node.get();

    node->next = std::move(head);
    head = std::move(node);
    if (head->next != nullptr) head->next->prev = head.get();
    ++elements_num;
    return iterator({raw});
}

template <typename T> anytypelist::iterator anytypelist::push_front(T &&value) {
    return emplace_front<std::decay_t<T>>(std::forward<T>(value));
}

} // namespace tools
