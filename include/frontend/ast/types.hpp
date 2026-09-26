#pragma once

#include <cstdint>
#include <format>
#include <string>

namespace frontend::ast::types {

#define EDGE_NAMES(...) enum edge_names { __VA_ARGS__ }

struct Root {
    EDGE_NAMES(STMT0);
};

struct StringLiteral {
    std::string value;

    StringLiteral(std::string_view sv);
};

struct CharLiteral {
    char value = 0;

    CharLiteral(std::string_view sv);
};

struct Literal {
    uint64_t value = 0;
    uint8_t size = 4;
    bool is_signed = true;

    Literal(std::string_view sv);
    Literal(uint64_t val, uint8_t size, bool is_signed) :
        value(val), size(size), is_signed(is_signed) {}

    /** apply an operator by it's string representation */
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
struct Subscript {
    EDGE_NAMES(VALUE, IDX);
};

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
    EDGE_NAMES(LEFT, RIGHT);

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
    EDGE_NAMES(DIM0);
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
struct Cast {
    EDGE_NAMES(TYPE, VALUE);
};

/** declare name as specific type */
struct Declare {
    EDGE_NAMES(TYPE);

    std::string name;

    Declare(std::string_view sv) : name(sv) {}
};

/** represents assignment operation of child 1 into child 0 */
struct Assign {
    EDGE_NAMES(LEFT, RIGHT);
};

/** child 0 is function name and type declaration. others are arguments */
struct Function {
    EDGE_NAMES(DECL, BODY, ARG0);
};

/** childs are statements */
struct Block {
    EDGE_NAMES(STMT0);
};

/** if (child0) child1 [else child2] */
struct If {
    EDGE_NAMES(COND, IF_STMT, ELSE_STMT);
};

/** for (child0; child1; child2) child3 */
struct For {
    EDGE_NAMES(INIT, COND, STEP, STMT);
};

/** initilaizers list for cycle base */
struct ForInit {
    EDGE_NAMES(STMT0);
};

/** expressions list for cycle step */
struct ForStep {
    EDGE_NAMES(STMT0);
};

/** return [child0] */
struct Return {};

/** call child0 with args child1, child2, ... */
struct Call {
    EDGE_NAMES(EXPR, ARG0);
};

struct Break {};

struct Continue {};

inline std::string to_string(const Root &) { return "root_node"; }
inline std::string to_string(const Literal &l) {
    return std::format(
        "{}: {}[{}]", l.is_signed ? "signed" : "unsigned", l.value, l.size
    );
}
inline std::string to_string(const StringLiteral &s) {
    return "\"" + s.value + "\"";
}
inline std::string to_string(const CharLiteral &c) {
    return std::string("'") + (char)c.value + "'";
}
inline std::string to_string(const ID &id) { return "id: " + id.name; }
inline std::string to_string(const Subscript &) { return "subscript"; }
inline std::string to_string(const Deref &) { return "deref"; }
inline std::string to_string(const Addr &) { return "addr"; }
inline std::string to_string(const Get &g) { return "get: " + g.name; }
inline std::string to_string(const BinaryOp &g) { return "binary " + g.op; }
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
inline std::string to_string(const Function &d) { return "function"; }
inline std::string to_string(const If &) { return "if"; }
inline std::string to_string(const For &) { return "for"; }
inline std::string to_string(const ForInit &) { return "init"; }
inline std::string to_string(const ForStep &) { return "step"; }
inline std::string to_string(const Return &) { return "return"; }
inline std::string to_string(const Call &) { return "call"; }
inline std::string to_string(const Block &) { return "block"; }
inline std::string to_string(const Break &) { return "break"; }
inline std::string to_string(const Continue &) { return "continue"; }

template <typename T> std::string edge_data_to_string(int data) {
    auto bad = [&] {
        return std::runtime_error(
            std::string("invalid edge index ") + std::to_string(data) + " for "
            + typeid(T).name()
        );
    };

    // clang-format off
    if constexpr (requires { T::LEFT; T::RIGHT; }) {
        if (data == T::LEFT)  return "left";
        if (data == T::RIGHT) return "right";
        throw bad();
    } else if constexpr (requires { T::VALUE; T::IDX; }) {
        if (data == T::VALUE) return "value";
        if (data == T::IDX)   return "idx";
        throw bad();
    } else if constexpr (requires { T::TYPE; T::VALUE; }) {
        if (data == T::TYPE)  return "type";
        if (data == T::VALUE) return "value";
        throw bad();
    } else if constexpr (requires { T::DECL; T::BODY; T::ARG0; }) {
        if (data == T::DECL) return "decl";
        if (data == T::BODY) return "body";
        if (data >= T::ARG0)
            return "arg" + std::to_string(data - T::ARG0);
        throw bad();
    } else if constexpr (requires { T::COND; T::IF_STMT; T::ELSE_STMT; }) {
        if (data == T::COND)      return "cond";
        if (data == T::IF_STMT)   return "if_stmt";
        if (data == T::ELSE_STMT) return "else_stmt";
        throw bad();
    } else if constexpr (requires { T::INIT; T::COND; T::STEP; T::STMT; }) {
        if (data == T::INIT) return "init";
        if (data == T::COND) return "cond";
        if (data == T::STEP) return "step";
        if (data == T::STMT) return "stmt";
        throw bad();
    } else if constexpr (requires { T::EXPR; T::ARG0; }) {
        if (data == T::EXPR) return "expr";
        if (data >= T::ARG0) return "arg" + std::to_string(data - T::ARG0);
        throw bad();
    } else if constexpr (requires { T::DIM0; }) {
        if (data >= T::DIM0) return "dim" + std::to_string(data - T::DIM0);
        throw bad();
    } else if constexpr (requires { T::STMT0; }) {
        if (data >= T::STMT0) return "stmt" + std::to_string(data - T::STMT0);
        throw bad();
    } else if constexpr (requires { T::TYPE; }) {
        if (data == T::TYPE)  return "type";
        throw bad();
    } else {
        if (data == 0) return "";
        throw bad();
    }
    // clang-format on
}

} // namespace frontend::ast::types
