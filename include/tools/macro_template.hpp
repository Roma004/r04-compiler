#pragma once

#include <type_traits>

#define typename_cond(_Type, cond) typename _Type, std::enable_if_t<cond>

#define typename_args_of(_Args, _Type) \
    typename... _Args,                 \
        std::enable_if_t<std::is_constructible_v<_Type, _Args...>, int>

#define typename_construct_assign(_Type)                    \
    typename _Type, std::enable_if_t<                       \
                        std::is_copy_constructible_v<_Type> \
                        && std::is_copy_assignable_v<_Type>>

#define HAS_NOEXCEPT_COPY(T)    std::is_nothrow_copy_constructible_v<T>
#define HAS_NOEXCEPT_MOVE(T)    std::is_nothrow_move_constructible_v<T>
#define HAS_NOEXCEPT_SWAP(T)    std::is_nothrow_swappable_v<T>
#define HAS_NOEXCEPT_DESTROY(T) std::is_nothrow_move_constructible_v<T>

namespace detail {
template <typename, template <typename...> class, typename... Args>
struct has_attr_impl : std::false_type {};

template <template <typename...> class Op, typename... Args>
struct has_attr_impl<std::void_t<Op<Args...>>, Op, Args...> : std::true_type {};

template <typename T>
using compare_operator =
    decltype(std::declval<const T &>().operator==(std::declval<const T &>()));

template <typename T>
using copy_operator =
    decltype(std::declval<T>().operator=(std::declval<const T &>()));

template <typename T>
using move_operator =
    decltype(std::declval<T>().operator=(std::declval<T &&>()));

template <typename T, typename _Ret>
using ret_type_is = std::enable_if_t<std::is_same_v<T, _Ret>>;

template <template <typename...> class Op, typename... Args>
using has_attr = detail::has_attr_impl<void, Op, Args...>;

template <template <typename...> class Op, typename... Args>
inline constexpr bool has_attr_v = has_attr<Op, Args...>::value;

} // namespace detail
