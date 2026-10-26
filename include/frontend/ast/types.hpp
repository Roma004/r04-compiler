#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "tools/macro_template.hpp"

namespace frontend::ast::types {

enum class NodeCategory { Expr, Stmt };

template <typename T>
inline std::string edge_name_impl(
    const T &node, int data, std::initializer_list<std::string_view> names
) {
    auto error = [&node, data]() {
        throw std::runtime_error(
            std::format("{}: invalid edge index {}", node.repr(), data)
        );
    };

    if (data < 0) error();
    if (names.size() == 0) {
        if (data == 0) return {};
        else error();
    }

    const std::string_view last = names.begin()[names.size() - 1];
    const bool has_prefix = !last.empty() && last.back() == '0';
    const int n_fixed = has_prefix ? names.size() - 1 : names.size();

    if (data < n_fixed) return helpers::to_lower_str(names.begin()[data]);

    if (!has_prefix) error();

    const std::string_view prefix = last.substr(0, last.size() - 1);
    return helpers::to_lower_str(prefix) + std::to_string(data - n_fixed);
}

#define AST_STR(name) #name

#define EDGE_NAMES(...)                                        \
    enum edge_names : int { __VA_ARGS__ };                     \
    std::string edge_name(int data) const {                    \
        return ::frontend::ast::types::edge_name_impl(         \
            *this, data, {FOR_EACH_CALL(AST_STR, __VA_ARGS__)} \
        );                                                     \
    }

#define AST_EXPR                                                     \
    static constexpr ::frontend::ast::types::NodeCategory category = \
        ::frontend::ast::types::NodeCategory::Expr
#define AST_STMT                                                     \
    static constexpr ::frontend::ast::types::NodeCategory category = \
        ::frontend::ast::types::NodeCategory::Stmt

template <typename T>
inline constexpr bool is_expr_v = [] {
    if constexpr (requires { T::category; })
        return T::category == NodeCategory::Expr;
    else return false;
}();

struct Root {
    AST_STMT;
    EDGE_NAMES(STMT0);

    std::string repr() const { return "root_node"; }
};

struct StringLiteral {
    AST_EXPR;
    EDGE_NAMES();

    std::string value;
    StringLiteral(std::string_view sv);

    std::string repr() const { return "\"" + value + "\""; }
};

struct CharLiteral {
    AST_EXPR;
    EDGE_NAMES();

    char value = 0;
    CharLiteral(std::string_view sv);

    std::string repr() const { return std::string("'") + value + "'"; }
};

struct Literal {
    AST_EXPR;
    EDGE_NAMES();

    uint64_t value = 0;
    uint8_t size = 4;
    bool is_signed = true;

    Literal(std::string_view sv);
    Literal(uint64_t val, uint8_t sz, bool s) :
        value(val), size(sz), is_signed(s) {}

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

    template <typename int_t> operator int_t() const {
        return static_cast<int_t>(value);
    }

    std::string repr() const {
        return std::format(
            "{}: {}[{}]", is_signed ? "signed" : "unsigned", value, size
        );
    }
};

struct ID {
    AST_EXPR;
    EDGE_NAMES();

    std::string name;
    ID(std::string_view sv) : name(sv) {}

    std::string repr() const { return "id: " + name; }
};

struct Subscript {
    AST_EXPR;
    EDGE_NAMES(VALUE, IDX);
    std::string repr() const { return "subscript"; }
};

struct Deref {
    AST_EXPR;
    EDGE_NAMES(BASE);

    std::string repr() const { return "deref"; }
};

struct Addr {
    AST_EXPR;
    EDGE_NAMES();

    std::string repr() const { return "addr"; }
};

struct Get {
    AST_EXPR;
    EDGE_NAMES(BASE);

    std::string name;
    Get(std::string_view sv) : name(sv) {}

    std::string repr() const { return "get: " + name; }
};

struct SizeOf {
    AST_EXPR;
    EDGE_NAMES(OF);

    std::string repr() const { return "sizeof"; }
};

struct Initializer {
    AST_EXPR;
    EDGE_NAMES(EXPR0);

    std::string repr() const { return "init_list"; }
};

struct BinaryOp {
    AST_EXPR;
    EDGE_NAMES(LEFT, RIGHT);

    std::string op;
    BinaryOp(std::string_view sv) : op(sv) {}

    std::string repr() const { return "binary " + op; }
};

struct UnaryOp {
    AST_EXPR;
    EDGE_NAMES(ARG);

    std::string op;
    UnaryOp(std::string_view sv) : op(sv) {}

    std::string repr() const { return "unary: " + op; }
};

struct TypeSpec {
    AST_STMT;
    EDGE_NAMES(DIM0);

    std::string name;
    bool is_const = false;

    TypeSpec(std::string_view sv) : name(sv) {}

    constexpr void add_const() noexcept { is_const = true; }

    std::string repr() const {
        if (is_const) return "const " + name;
        return name;
    }
};

struct CallSign {
    AST_STMT;
    EDGE_NAMES(ARG0);

    std::string repr() const { return "call_types"; }
};

struct Ptr {
    AST_STMT;
    EDGE_NAMES();

    bool is_const = false;

    std::string repr() const { return "ptr"; }
};

struct Arr {
    AST_STMT;
    EDGE_NAMES(SIZE);

    bool is_const = false;

    std::string repr() const { return "arr"; }
};

struct Cast {
    AST_EXPR;
    EDGE_NAMES(TYPE, VALUE);

    std::string repr() const { return "cast"; }
};

struct Declare {
    AST_EXPR;
    EDGE_NAMES(TYPE);

    std::string name;
    Declare(std::string_view sv) : name(sv) {}

    std::string repr() const { return "declare: " + name; }
};

struct Assign {
    AST_STMT;
    EDGE_NAMES(LEFT, RIGHT);

    std::string repr() const { return "="; }
};

struct Function {
    AST_STMT;
    EDGE_NAMES(DECL, BODY, ARG0);

    std::string repr() const { return "function"; }
};

struct Structure {
    AST_STMT;
    EDGE_NAMES(TYPE, FIELD0);

    bool has_body = false;
    std::string repr() const { return "struct"; }
};

struct Block {
    AST_STMT;
    EDGE_NAMES(STMT0);
    std::string repr() const { return "block"; }
};

struct If {
    AST_STMT;
    EDGE_NAMES(COND, IF_STMT, ELSE_STMT);

    std::string repr() const { return "if"; }
};

struct For {
    AST_STMT;
    EDGE_NAMES(INIT, COND, STEP, STMT);

    std::string repr() const { return "for"; }
};

struct ForInit {
    AST_STMT;
    EDGE_NAMES(STMT0);

    std::string repr() const { return "init"; }
};

struct ForStep {
    AST_STMT;
    EDGE_NAMES(STMT0);

    std::string repr() const { return "step"; }
};

struct Return {
    AST_STMT;
    EDGE_NAMES();

    std::string repr() const { return "return"; }
};

struct Call {
    AST_EXPR;
    EDGE_NAMES(EXPR, ARG0);

    std::string repr() const { return "call"; }
};

struct Break {
    AST_STMT;
    EDGE_NAMES();

    std::string repr() const { return "break"; }
};

struct Continue {
    AST_STMT;
    EDGE_NAMES();

    std::string repr() const { return "continue"; }
};

} // namespace frontend::ast::types