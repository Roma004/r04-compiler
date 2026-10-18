#pragma once

#include <concepts>
#include <utility>
#include <variant>

#include <frontend/ast/type_info.hpp>
#include <frontend/ast/types.hpp>
#include <frontend/parser/source_map.hpp>
#include <tools/macro_template.hpp>
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
    template <typename T> bool is() const {
        return std::holds_alternative<T>(val);
    }

    std::string repr(const TypeTable &tb) const;

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
        types::Const,
        types::Continue,
        types::Structure,
        types::Get>
        val;
    parser::SourceLocation loc = {};
    std::optional<node_type> type;
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

    TypeTable types;

    std::string repr_node(const const_node_iter &node) const;
    std::string repr_edge(const const_edge_iter &node) const;
};

}; // namespace frontend::ast
