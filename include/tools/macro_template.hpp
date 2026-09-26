#pragma once

#include <concepts>
#include <format>
#include <string>
#include <vector>

#define HAS_NOEXCEPT_COPY(T)    std::is_nothrow_copy_constructible_v<T>
#define HAS_NOEXCEPT_MOVE(T)    std::is_nothrow_move_constructible_v<T>
#define HAS_NOEXCEPT_SWAP(T)    std::is_nothrow_swappable_v<T>
#define HAS_NOEXCEPT_DESTROY(T) std::is_nothrow_move_constructible_v<T>

#define FOR_EACH_ONE(pref, suf, name) pref##name##suf

#define FE_1(P, S, x)       FOR_EACH_ONE(P, S, x)
#define FE_2(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_1(P, S, __VA_ARGS__)
#define FE_3(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_2(P, S, __VA_ARGS__)
#define FE_4(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_3(P, S, __VA_ARGS__)
#define FE_5(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_4(P, S, __VA_ARGS__)
#define FE_6(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_5(P, S, __VA_ARGS__)
#define FE_7(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_6(P, S, __VA_ARGS__)
#define FE_8(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_7(P, S, __VA_ARGS__)
#define FE_9(P, S, x, ...)  FOR_EACH_ONE(P, S, x), FE_8(P, S, __VA_ARGS__)
#define FE_10(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_9(P, S, __VA_ARGS__)
#define FE_11(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_10(P, S, __VA_ARGS__)
#define FE_12(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_11(P, S, __VA_ARGS__)
#define FE_13(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_12(P, S, __VA_ARGS__)
#define FE_14(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_13(P, S, __VA_ARGS__)
#define FE_15(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_14(P, S, __VA_ARGS__)
#define FE_16(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_15(P, S, __VA_ARGS__)
#define FE_17(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_16(P, S, __VA_ARGS__)
#define FE_18(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_17(P, S, __VA_ARGS__)
#define FE_19(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_18(P, S, __VA_ARGS__)
#define FE_20(P, S, x, ...) FOR_EACH_ONE(P, S, x), FE_19(P, S, __VA_ARGS__)

// clang-format off
#define NARG(...)                               \
    NARG_(                                      \
        __VA_ARGS__,                            \
        20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
        10,  9,  8,  7,  6,  5,  4,  3,  2,  1  \
    )

#define NARG_(                                        \
    _1,   _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    N, ...                                            \
) N
// clang-fromat on

#define CAT(a, b)  CAT_(a, b)
#define CAT_(a, b) a##b

#define FOR_EACH(pref, suf, ...) \
    CAT(FE_, NARG(__VA_ARGS__))(pref, suf, __VA_ARGS__)

namespace helpers {
using std::to_string;

template <typename T>
concept Repersentable = requires(const T &obj) {
    { obj.repr() } -> std::convertible_to<std::string>;
};

template <typename T>
concept Stringifiable = requires(const T &obj) {
    { to_string(obj) } -> std::convertible_to<std::string>;
};

using DOTList = std::vector<std::pair<std::string, std::string>>;

template <typename T, typename Conv>
concept DOTConverter = requires(const Conv &c, const T &val) {
    { c(val) } -> std::convertible_to<DOTList>;
};

} // namespace helpers

namespace std {

template <helpers::Repersentable T>
struct formatter<T, char> : formatter<string, char> {
    template <typename Ctx> auto format(const T &v, Ctx &ctx) const {
        return formatter<string, char>::format(std::string(v.repr()), ctx);
    }
};

} // namespace std
