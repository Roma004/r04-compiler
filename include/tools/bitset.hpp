#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

namespace tools {

template <size_t size> struct uint_t;

// clang-format off
template <> struct uint_t<8>  { using type = uint8_t;  };
template <> struct uint_t<16> { using type = uint16_t; };
template <> struct uint_t<32> { using type = uint32_t; };
template <> struct uint_t<64> { using type = uint64_t; };
// clang-format on

/**
 * @brief A fixed-width bit set backed by a single unsigned integer
 * @tparam width  Number of bits in the set (8, 16, 32, or 64)
 */
template <size_t size> struct bitset {
    using int_type = typename uint_t<size>::type;

    /** @brief Checks whether every bit is set */
    constexpr bool is_full() const noexcept;

    /** @brief Checks whether every bit is clear */
    constexpr bool is_empty() const noexcept;

    /**
     * @brief Finds the index of the first free bit
     * @return Index of the first zero bit, or -1 if the set is full
     */
    constexpr ssize_t find_free_idx() const noexcept;

    /**
     * @brief Finds the index of the first used bit
     * @return Index of the first zero bit, or -1 if the set is empty
     */
    constexpr ssize_t find_used_idx() const noexcept;

    /** @brief Reads the state of a single bit */
    constexpr bool is_used(size_t idx) const noexcept;

    constexpr void set(size_t idx) noexcept;
    constexpr void unset(size_t idx) noexcept;

    /** @brief Returns the number of used bits */
    constexpr unsigned count() const noexcept;

    /** @brief Unset all elements */
    constexpr void clear() noexcept;

    int_type val = 0;
};

template <size_t size>
constexpr unsigned bitset<size>::count() const noexcept {
    return __builtin_popcountg(val);
}

template <size_t size> constexpr void bitset<size>::clear() noexcept {
    val = 0;
}

template <size_t size> constexpr bool bitset<size>::is_full() const noexcept {
    return val == (int_type)-1;
}

template <size_t size> constexpr bool bitset<size>::is_empty() const noexcept {
    return val == (int_type)0;
}

template <size_t size>
constexpr ssize_t bitset<size>::find_free_idx() const noexcept {
    if (is_full()) return -1;
    return __builtin_ctzg((int_type)~val);
}

template <size_t size>
constexpr ssize_t bitset<size>::find_used_idx() const noexcept {
    if (is_empty()) return -1;
    return __builtin_ctzg(val);
}

template <size_t size>
constexpr bool bitset<size>::is_used(size_t idx) const noexcept {
    return (int_type)(val >> idx) & 1;
}

template <size_t size> constexpr void bitset<size>::set(size_t idx) noexcept {
    val |= int_type(1) << idx;
}

template <size_t size> constexpr void bitset<size>::unset(size_t idx) noexcept {
    val &= ~(int_type(1) << idx);
}

} // namespace tools
