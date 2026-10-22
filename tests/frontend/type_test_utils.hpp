#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "frontend/ast/ast.hpp"
#include "frontend/parser/context.hpp"

namespace type_test {

using ast_t = frontend::ast::Ast;
using node_iter = ast_t::node_iter;
namespace ast_types = frontend::ast::types;

inline std::string read_file(const std::filesystem::path &path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline std::filesystem::path inputs_dir() {
    return std::filesystem::path(TEST_SOURCE_DIR) / "tests" / "frontend"
         / "inputs";
}

inline node_iter child(node_iter parent, int data) {
    for (auto edge : parent->forward_edges())
        if (edge->get_data().data == data) return edge->target();
    FAIL("edge with data " << data << " not found");
    return parent;
}

template <typename Fn> inline void visit_nodes(node_iter root, Fn &&fn) {
    fn(root);
    for (auto edge : root->forward_edges()) visit_nodes(edge->target(), fn);
}

inline bool is_expr_node(const frontend::ast::node_t &data) {
    return std::visit(
        [](const auto &value) {
            using value_t = std::remove_cvref_t<decltype(value)>;
            return ast_types::is_expr_v<value_t>;
        },
        data.val
    );
}

inline node_iter find_function(node_iter root, std::string_view name) {
    node_iter result = root;
    bool found = false;
    visit_nodes(root, [&](node_iter node) {
        auto &data = node->get_data();
        if (!data.is<ast_types::Function>()) return;
        node_iter decl = child(node, ast_types::Function::DECL);
        auto &decl_data = decl->get_data();
        if (decl_data.is<ast_types::Declare>()
            && decl_data.get<ast_types::Declare>().name == name) {
            result = node;
            found = true;
        }
    });
    REQUIRE(found);
    return result;
}

inline std::string prelude() {
    return R"(
struct inner_t {
    int32_t a;
}

struct point_t {
    int32_t x;
    int32_t y;
    inner_t inner;
}

void_t noop(int32_t x);
int32_t id_fn();
uint64_t u64_fn();

int8_t i8;
int16_t i16;
int32_t i32;
int64_t i64;
uint8_t u8;
uint16_t u16;
uint32_t u32;
uint64_t u64;
bool_t flag;
const int32_t ci = 1;
int32_t *pi;
int32_t[4] arr;
int32_t[2][3] matrix;
point_t pt;
point_t *ppt;
inner_t inner_val;
const int8_t *str;
)";
}

/** @brief type of @p expr as it is deduced in the context of @p source */
inline std::string
expr_type_in(std::string_view source, std::string_view expr) {
    INFO("expression: " << expr);

    std::string src(source);
    src += "\nint32_t main() { return ";
    src.append(expr);
    src += "; }";

    frontend::parser::ParserContext ctx(src);
    REQUIRE(ctx.parse());
    REQUIRE(ctx.ast.check_semantics(ctx));
    REQUIRE(ctx.errors.empty());

    node_iter main_fn = find_function(ctx.ast.get_root(), "main");
    node_iter body = child(main_fn, ast_types::Function::BODY);
    REQUIRE_FALSE(body->forward_edges().empty());

    node_iter ret = body->forward_edges().front()->target();
    REQUIRE(ret->get_data().is<ast_types::Return>());
    REQUIRE(ret->forward_edges().size() == 1);

    node_iter expr_node = ret->forward_edges().front()->target();
    const auto &type = expr_node->get_data().type;
    REQUIRE(type.has_value());
    return type->repr(ctx.ast.types);
}

inline void check_expr_type(std::string_view expr, std::string_view expected) {
    INFO("expression: " << expr);
    REQUIRE(expr_type_in(prelude(), expr) == expected);
}

inline void check_valid(std::string_view source) {
    INFO("source:\n" << source);

    frontend::parser::ParserContext ctx(source);
    REQUIRE(ctx.parse());
    REQUIRE(ctx.ast.check_semantics(ctx));
    REQUIRE(ctx.errors.empty());
}

/** @brief type of the variable @p name declared in @p source */
inline std::string
decl_type_in(std::string_view source, std::string_view name) {
    INFO("declaration: " << name);

    frontend::parser::ParserContext ctx(source);
    REQUIRE(ctx.parse());
    REQUIRE(ctx.ast.check_semantics(ctx));
    REQUIRE(ctx.errors.empty());

    std::string result;
    bool found = false;
    visit_nodes(ctx.ast.get_root(), [&](node_iter node) {
        auto &data = node->get_data();
        if (!data.is<ast_types::Declare>()) return;
        if (data.get<ast_types::Declare>().name != name) return;
        REQUIRE(data.type.has_value());
        result = data.type->repr(ctx.ast.types);
        found = true;
    });
    REQUIRE(found);
    return result;
}

inline void
check_invalid(std::string_view source, std::string_view expected_error) {
    INFO("source:\n" << source);

    frontend::parser::ParserContext ctx(source);
    REQUIRE(ctx.parse());
    REQUIRE_FALSE(ctx.ast.check_semantics(ctx));
    REQUIRE_FALSE(ctx.errors.empty());
    REQUIRE(ctx.errors.front().msg.find(expected_error) != std::string::npos);
}

}; // namespace type_test
