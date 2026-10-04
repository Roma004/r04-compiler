#pragma once

#include <concepts>
#include <format>
#include <string>
#include <vector>

#define HAS_NOEXCEPT_COPY(T)    std::is_nothrow_copy_constructible_v<T>
#define HAS_NOEXCEPT_MOVE(T)    std::is_nothrow_move_constructible_v<T>
#define HAS_NOEXCEPT_SWAP(T)    std::is_nothrow_swappable_v<T>
#define HAS_NOEXCEPT_DESTROY(T) std::is_nothrow_move_constructible_v<T>

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

inline std::string dot_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + s.size() / 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

} // namespace helpers

namespace std {

template <helpers::Repersentable T>
struct formatter<T, char> : formatter<string, char> {
    template <typename Ctx> auto format(const T &v, Ctx &ctx) const {
        return formatter<string, char>::format(std::string(v.repr()), ctx);
    }
};

} // namespace std
