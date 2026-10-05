#pragma once

#include <concepts>
#include <utility>
#include <variant>

#include <frontend/ast/types.hpp>
#include <frontend/parser/source_map.hpp>
#include <tools/tree.hpp>

namespace frontend::parser {
struct ParserContext;
};

namespace frontend::ast {

struct node_t {
    template <typename T>
    node_t(const parser::SourceLocation &loc, T &&val) :
        val(std::move(val)), loc(loc) {}

    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    static node_t create(const parser::SourceLocation &loc, Args &&...args) {
        return node_t(loc, T(std::forward<Args &&>(args)...));
    }

    static node_t create_root() { return node_t({}, types::Root{}); }

    template <typename T> T &get() { return std::get<T>(val); }
    template <typename T> const T &get() const { return std::get<T>(val); }

    template <typename T> bool is() { return std::holds_alternative<T>(val); }

    std::variant<
        types::Root,
        types::CharLiteral,
        types::StringLiteral,
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
        types::Function,
        types::If,
        types::For,
        types::ForInit,
        types::ForStep,
        types::Return,
        types::Call,
        types::Block,
        types::Break,
        types::Continue,
        types::Get>
        val;
    parser::SourceLocation loc = {};
};

struct edge_t {
    int data;
};

using ast_tree_t = tools::tree_t<node_t, edge_t>;

class Ast : public ast_tree_t {
  public:
    using node_iter = tree_t::node_iter;
    using edge_iter = tree_t::edge_iter;
    using const_node_iter = tree_t::const_node_iter;
    using const_edge_iter = tree_t::const_edge_iter;

    bool check_semantics(parser::ParserContext &);
};

inline std::string to_string(const Ast::const_node_iter &node) {
    using std::to_string;
    return std::visit(
        [](const auto &v) { return to_string(v); }, node->get_data().val
    );
}

inline std::string to_string(const Ast::const_edge_iter &edge) {
    const node_t &node = edge->source()->get_data();
    return std::visit(
        [&edge](const auto &v) {
            return types::edge_data_to_string<std::remove_cvref_t<decltype(v)>>(
                edge->get_data().data
            );
        },
        node.val
    );
}

}; // namespace frontend::ast
