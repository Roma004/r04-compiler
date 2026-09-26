#pragma once

#include <cstdint>
#include <format>
#include <string>

namespace frontend::ast::types {

struct Root {};

/** numeric literal */
struct Literal {
    uint64_t value = 0;
    uint8_t size = 4;
    bool is_signed = true;

    Literal(std::string_view sv);
    uint64_t mask_for_size(uint8_t sz);
};

/** variable or function name */
struct ID {
    std::string name;

    ID(std::string_view sv) : name(sv) {}
};

/** get element of child 0 by child 1 as index */
struct Subscript {};

/** dereference child 0 */
struct Deref {};

/** get address of child 0 */
struct Addr {};

/** get member NAME of child 0 */
struct Get {
    std::string name;

    Get(std::string_view sv) : name(sv) {}
};

inline std::string to_string(const Root &) { return "root_node"; }
inline std::string to_string(const Literal &l) {
    return std::format(
        "{}: {}[{}]", l.is_signed ? "signed" : "unsigned", l.value, l.size
    );
}
inline std::string to_string(const ID &id) { return "id: " + id.name; }
inline std::string to_string(const Subscript &) { return "subscript"; }
inline std::string to_string(const Deref &) { return "deref"; }
inline std::string to_string(const Addr &) { return "addr"; }
inline std::string to_string(const Get &g) { return "get: " + g.name; }

} // namespace frontend::ast::types
