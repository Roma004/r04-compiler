#pragma once

#include "tools/macro_template.hpp"
#include <format>
#include <stdexcept>

namespace contract::hir {

// typed reference to the HIR object
struct Value {
    enum class Kind : uint8_t { object, constant, label, function };
    unsigned id = 0;
    Kind kind = Kind::object;

    static Value obj(unsigned i) { return {i, Kind::object}; }
    static Value cst(unsigned i) { return {i, Kind::constant}; }
    static Value lbl(unsigned i) { return {i, Kind::label}; }
    static Value fn(unsigned i) { return {i, Kind::function}; }

    void check_is(Kind k, std::string_view top_msg) {
        static auto strkind = [](Kind kind) {
            switch (kind) {
            case Kind::constant: return "constant";
            case Kind::label: return "label";
            case Kind::function: return "function";
            case Kind::object: return "object";
            }
        };
        if (k != kind) {
            throw std::logic_error(
                std::string(top_msg)
                + std::format("expected {}, got {}", strkind(k), strkind(kind))
            );
        }
    }
};

} // namespace contract::hir

#define HIR_OBJECT(name)   (::contract::hir::Value::Kind::object, name)
#define HIR_CONSTANT(name) (::contract::hir::Value::Kind::constant, name)
#define HIR_LABEL(name)    (::contract::hir::Value::Kind::label, name)
#define HIR_FUNCTION(name) (::contract::hir::Value::Kind::function, name)

#define HIR_ARG_DECL(x)               HIR_ARG_DECL_IMPL x
#define HIR_ARG_DECL_IMPL(kind, name) name

#define HIR_ARG_CHECK(x) HIR_ARG_CHECK_IMPL x
#define HIR_ARG_CHECK_IMPL(kind, name) \
    name.check_is(kind, std::string(top) + ": `" #name "` ")

#define HIR_ARGS(...)                                                \
    ::contract::hir::Value FOR_EACH_CALL(HIR_ARG_DECL, __VA_ARGS__); \
    void check_args(std::string_view top) {                          \
        FOR_EACH_CALL(HIR_ARG_CHECK, __VA_ARGS__);                   \
    }

namespace contract::hir::ops {

// clang-format off
struct Const { HIR_ARGS(HIR_OBJECT(dst), HIR_CONSTANT(value)) };

struct Add { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Sub { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Mul { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Div { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Mod { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct And { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Or  { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Xor { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Shl { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Shr { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };

struct Neg { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(src)) };
struct Not { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(src)) };
struct Inv { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(src)) };

struct Eq { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Ne { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Lt { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Le { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Gt { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };
struct Ge { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(lhs), HIR_OBJECT(rhs)) };

struct AddrOf   { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(src)) };
struct Load     { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(ptr)) };
struct Store    { HIR_ARGS(HIR_OBJECT(ptr), HIR_OBJECT(value)) };
struct ElemPtr  { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(arr), HIR_OBJECT(idx)) };
struct FieldPtr { HIR_ARGS(
    HIR_OBJECT(dst), HIR_OBJECT(ptr), HIR_CONSTANT(field_idx)
) };

struct Cast { HIR_ARGS(HIR_OBJECT(dst), HIR_OBJECT(src)) };

struct Label { HIR_ARGS(HIR_LABEL(label)) };
struct Jump { HIR_ARGS(HIR_LABEL(label)) };
struct Branch { HIR_ARGS(
    HIR_OBJECT(cond), HIR_LABEL(true_label), HIR_LABEL(false_label)
) };

struct Call {
    HIR_ARGS(HIR_FUNCTION(fn), HIR_OBJECT(dst))
    std::vector<Value> varargs;

    void check_varargs(std::string_view top) {
        for (std::size_t i = 0; i < varargs.size(); ++i)
            varargs[i].check_is(
                Value::Kind::object,
                std::string(top) + ": vararg[" + std::to_string(i) + "] "
            );
    }
};

struct Return {
    std::vector<Value> varargs;

    void check_args(std::string_view top) { (void) top; }

    void check_varargs(std::string_view top) {
        if (varargs.size() > 1)
            throw std::logic_error(
                std::string(top) + ": return takes 0 or 1 argument"
            );
        for (std::size_t i = 0; i < varargs.size(); ++i)
            varargs[i].check_is(
                Value::Kind::object,
                std::string(top) + ": return value "
            );
    }
};

#undef HIR_OBJECT
#undef HIR_CONSTANT
#undef HIR_LABEL
#undef HIR_FUNCTION
#undef HIR_ARG_DECL
#undef HIR_ARG_DECL_IMPL
#undef HIR_ARG_CHECK
#undef HIR_ARG_CHECK_IMPL
#undef HIR_ARGS

// clang-format on

}; // namespace contract::hir::ops