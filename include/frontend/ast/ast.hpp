#pragma once

#include <variant>

#include <frontend/ast/source_map.hpp>
#include <frontend/ast/types.hpp>
#include <tools/tree.hpp>

namespace frontend::ast {

struct TypeInfo {
    enum {
        SIGNED = 1 << 0,
        CONST = 1 << 1,
        VOLATILE = 1 << 2,
    };
    std::string type;
    unsigned attrs = 0;

    TypeInfo(std::string &&type) : type(std::move(type)) {}
};

struct TypeDecl {
    std::string name;
    SourceLocation loc;
};

struct VarDecl {
    TypeInfo type;
    std::string name;
    SourceLocation loc;
};

struct node_t {
    template <typename T>
    static node_t from_sv(std::string_view sv, SourceLocation &&loc) {
        return node_t{.val = T(sv), .loc = std::move(loc)};
    }

    template <typename T> static node_t from_loc(const SourceLocation &loc) {
        return node_t{.val = T(), .loc = loc};
    }

    std::variant<
        types::Root,
        types::Literal,
        types::ID,
        types::Subscript,
        types::Deref,
        types::Addr,
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
