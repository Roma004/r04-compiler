#pragma once

#include <concepts>
#include <utility>
#include <variant>

#include <frontend/ast/source_map.hpp>
#include <frontend/ast/types.hpp>
#include <tools/tree.hpp>

namespace frontend::ast {

struct node_t {
    template <typename T>
    node_t(const SourceLocation &loc, T &&val) :
        val(std::move(val)), loc(loc) {}

    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    static node_t create(const SourceLocation &loc, Args &&...args) {
        return node_t(loc, T(std::forward<Args &&>(args)...));
    }

    static node_t create_root() { return node_t({}, types::Root{}); }

    template <typename T> T &get() { return std::get<T>(val); }
    template <typename T> const T &get() const { return std::get<T>(val); }

    std::variant<
        types::Root,
        types::Literal,
        types::ID,
        types::BinaryOp,
        types::UnaryOp,
        types::Subscript,
        types::Deref,
        types::Addr,
        types::Cast,
        types::Declare,
        types::TypeSpec,
        types::Pointer,
        types::Assign,
        types::Get>
        val;
    SourceLocation loc = {};
};

inline std::string to_string(const node_t &node) {
    using std::to_string;
    return std::visit([](const auto &v) { return to_string(v); }, node.val);
}

using ast_tree_t = tools::tree_t<node_t, int>;

class Ast : public ast_tree_t {
  public:
    using node_iter = tree_t::node_iter;
};

}; // namespace frontend::ast
