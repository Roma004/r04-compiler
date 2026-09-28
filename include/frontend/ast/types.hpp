#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <variant>
#include <vector>

namespace frontend::ast::types {

struct Root {};

/** numeric literal */
struct Literal {
    uint64_t value = 0;
    uint8_t size = 4;
    bool is_signed = true;

    Literal(std::string_view sv);
    Literal(uint64_t val, uint8_t size, bool is_signed) :
        value(val), size(size), is_signed(is_signed) {}

    /** apply an operator ny it's string representation */
    Literal apply(std::string_view op, const Literal &other) const;
    Literal apply(std::string_view op) const;

    static uint64_t mask_for_size(uint8_t sz);
    static int64_t sign_extend(uint64_t v, uint8_t sz);
    static uint64_t convert_to(
        uint64_t val,
        uint8_t from_size,
        bool from_signed,
        uint8_t to_size,
        bool to_signed
    );
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

/** apply binary operation between child 0 and child 1 */
struct BinaryOp {
    std::string op;

    BinaryOp(std::string_view sv) : op(sv) {}
};

/** apply unary operation to child 0 */
struct UnaryOp {
    std::string op;

    UnaryOp(std::string_view sv) : op(sv) {}
};

/** represents typename with qualifiers like const or volatile */
struct TypeSpec {
    enum OFST {
        CONST_OFST = 1,
        VOLATILE_OFST = 2,
        SIGNED_OFST = 3,
        INTEGRAL_OFST = 4,
    };
    enum MASK {
        CONST = 1 << OFST::CONST_OFST,
        VOLATILE = 1 << OFST::VOLATILE_OFST,
        SIGNED = 1 << OFST::SIGNED_OFST,
        INTEGRAL = 1 << OFST::INTEGRAL_OFST,
    };
    enum { PTR = -1 };
    std::string name;
    unsigned size = 1;
    uint8_t attrs = 0;

    TypeSpec(std::string_view sv);

    constexpr bool is_signed() const noexcept {
        return (attrs & SIGNED) >> SIGNED_OFST;
    }
    constexpr bool is_volatile() const noexcept {
        return (attrs & VOLATILE) >> VOLATILE_OFST;
    }
    constexpr bool is_const() const noexcept {
        return (attrs & CONST) >> CONST_OFST;
    }
    constexpr bool is_intagral() const noexcept {
        return (attrs & INTEGRAL) >> INTEGRAL_OFST;
    }

    constexpr void add_volatile() noexcept { attrs |= VOLATILE; }
    constexpr void add_const() noexcept { attrs |= CONST; }
};

struct Pointer {};

/** cast child 0 is cast-type, child 1 is expression to cast */
struct Cast {};

/** declare name as specific type. childs are array lengths */
struct Declare {
    std::string name;

    Declare(std::string_view sv) : name(sv) {}
};

/** represents assignment operation of child 1 into child 0 */
struct Assign {};

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
inline std::string to_string(const BinaryOp &g) { return "banary " + g.op; }
inline std::string to_string(const UnaryOp &g) { return "unary: " + g.op; }
inline std::string to_string(const TypeSpec &g) {
    std::string res;
    if (g.attrs & TypeSpec::CONST) res += "const ";
    if (g.attrs & TypeSpec::VOLATILE) res += "volatile ";
    return res + g.name;
}
inline std::string to_string(const Declare &d) { return "declare: " + d.name; }
inline std::string to_string(const Cast &d) { return "cast"; }
inline std::string to_string(const Pointer &d) { return "ptr"; }
inline std::string to_string(const Assign &d) { return "="; }

} // namespace frontend::ast::types
