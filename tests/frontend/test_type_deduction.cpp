#include <filesystem>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "type_test_utils.hpp"

using namespace type_test;

template <typename Fn> void visit_value_nodes(node_iter root, Fn &&fn) {
    fn(root);
    if (root->get_data().is<ast_types::TypeSpec>()) return;
    for (auto edge : root->forward_edges())
        visit_value_nodes(edge->target(), fn);
}

TEST_CASE(
    "check_semantics annotates every expression node", "[semantic][types]"
) {
    const std::filesystem::path ok_dir = inputs_dir() / "ok";

    for (const auto &entry : std::filesystem::directory_iterator(ok_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        const std::string src = read_file(entry.path());
        frontend::parser::ParserContext ctx(src);
        REQUIRE(ctx.parse());
        REQUIRE(ctx.ast.check_semantics(ctx));
        REQUIRE(ctx.errors.empty());

        visit_value_nodes(ctx.ast.get_root(), [&](node_iter node) {
            if (!is_expr_node(node->get_data())) return;
            INFO("expression node at line " << node->get_data().loc.line);
            REQUIRE(node->get_data().type.has_value());
        });
    }
}

TEST_CASE("literal and identifier type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"2", "int32_t"},
        {"2u", "uint32_t"},
        {"2l", "int64_t"},
        {"2ul", "uint64_t"},
        {"0xFF", "int32_t"},
        {"0b101", "int32_t"},
        {"017", "int32_t"},
        {"'a'", "int8_t"},
        {"\"str\"", "int8_t const *"},
        {"i8", "int8_t"},
        {"i16", "int16_t"},
        {"i32", "int32_t"},
        {"i64", "int64_t"},
        {"u8", "uint8_t"},
        {"u16", "uint16_t"},
        {"u32", "uint32_t"},
        {"u64", "uint64_t"},
        {"flag", "bool_t"},
        {"ci", "int32_t const"},
        {"pi", "int32_t *"},
        {"arr", "int32_t [4]"},
        {"matrix", "int32_t [2] [3]"},
        {"pt", "point_t"},
        {"ppt", "point_t *"},
        {"inner_val", "inner_t"},
        {"str", "int8_t const *"},
        {"id_fn", "int32_t ()"},
        {"noop", "void_t (int32_t)"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("arithmetic type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"2u + (-1)", "uint32_t"}, {"i8 + i8", "int32_t"},
        {"i16 + i16", "int32_t"},  {"u8 + u8", "int32_t"},
        {"u16 + u16", "int32_t"},  {"flag + flag", "int32_t"},
        {"i8 + i16", "int32_t"},   {"u8 + i16", "int32_t"},
        {"i32 + i64", "int64_t"},  {"u32 + i64", "int64_t"},
        {"i64 + u32", "int64_t"},  {"u64 + i64", "uint64_t"},
        {"u32 + i32", "uint32_t"}, {"i32 + u64", "uint64_t"},
        {"i8 * i64", "int64_t"},   {"u8 - i32", "int32_t"},
        {"i32 % u32", "uint32_t"}, {"i32 & u64", "uint64_t"},
        {"u16 | i64", "int64_t"},  {"i8 ^ u8", "int32_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}
