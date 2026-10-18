#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <frontend/ast/ast.hpp>
#include <frontend/parser/context.hpp>

namespace {

using ast_t = frontend::ast::Ast;
using node_iter = ast_t::node_iter;

std::string read_file(const std::filesystem::path &path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::filesystem::path inputs_dir() {
    return std::filesystem::path(TEST_SOURCE_DIR) / "tests" / "frontend"
         / "inputs";
}

node_iter child(node_iter parent, int data) {
    for (auto edge : parent->forward_edges())
        if (edge->get_data().data == data) return edge->target();
    FAIL("edge with data " << data << " not found");
    return parent;
}

template <typename Fn> void visit_nodes(node_iter root, Fn &&fn) {
    fn(root);
    for (auto edge : root->forward_edges()) visit_nodes(edge->target(), fn);
}

} // namespace

TEST_CASE("check_semantics accepts every valid example", "[semantic]") {
    const std::filesystem::path ok_dir = inputs_dir() / "ok";

    for (const auto &entry : std::filesystem::directory_iterator(ok_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        const std::string src = read_file(entry.path());
        frontend::parser::ParserContext ctx(src);
        REQUIRE(ctx.parse());
        REQUIRE(ctx.ast.check_semantics(ctx));
        REQUIRE(ctx.errors.empty());
    }
}

TEST_CASE("check_semantics rejects invalid examples", "[semantic]") {
    struct case_t {
        const char *filename;
        const char *expected_error;
    };
    const std::vector<case_t> cases = {
        {"assign_to_undefined.r04", "undefined name `x`"},
        {"call_target_not_id.r04", "call target must be an identifier"},
        {"redefined_function.r04", "redefinition of function `f`"},
        {"redefined_name.r04", "redefinition of name `a`"},
        {"undefined_name.r04", "undefined name `x`"},
        {"undefined_type.r04", "type foo_t is not defined"},
        {"unknown_function.r04", "unknown function `foo`"},
    };

    const std::filesystem::path bad_dir = inputs_dir() / "bad" / "semantic";

    for (const auto &test : cases) {
        INFO("file: " << test.filename);
        const std::string src = read_file(bad_dir / test.filename);
        frontend::parser::ParserContext ctx(src);
        REQUIRE(ctx.parse());
        REQUIRE_FALSE(ctx.ast.check_semantics(ctx));
        REQUIRE_FALSE(ctx.errors.empty());
        REQUIRE(
            ctx.errors.front().msg.find(test.expected_error)
            != std::string::npos
        );
    }
}

TEST_CASE("AST shape after parse", "[semantic][ast]") {
    const std::string src = "int32_t main() { int32_t x = 1; return x; }\n";
    frontend::parser::ParserContext ctx(src);
    REQUIRE(ctx.parse());

    node_iter root = ctx.ast.get_root();
    REQUIRE(root->get_data().is<frontend::ast::types::Root>());
    REQUIRE(root->forward_edges().size() == 1);

    node_iter fn = root->forward_edges().front()->target();
    REQUIRE(fn->get_data().is<frontend::ast::types::Function>());

    node_iter decl = child(fn, frontend::ast::types::Function::DECL);
    REQUIRE(decl->get_data().is<frontend::ast::types::Declare>());
    REQUIRE(
        decl->get_data().get<frontend::ast::types::Declare>().name == "main"
    );

    node_iter body = child(fn, frontend::ast::types::Function::BODY);
    REQUIRE(body->get_data().is<frontend::ast::types::Block>());
    REQUIRE(body->forward_edges().size() == 2);

    node_iter stmt0 = body->forward_edges()[0]->target();
    REQUIRE(stmt0->get_data().is<frontend::ast::types::Assign>());

    node_iter decl_x = child(stmt0, frontend::ast::types::Assign::LEFT);
    REQUIRE(decl_x->get_data().is<frontend::ast::types::Declare>());
    REQUIRE(
        decl_x->get_data().get<frontend::ast::types::Declare>().name == "x"
    );

    node_iter init = child(stmt0, frontend::ast::types::Assign::RIGHT);
    REQUIRE(init->get_data().is<frontend::ast::types::Literal>());

    node_iter ret = body->forward_edges()[1]->target();
    REQUIRE(ret->get_data().is<frontend::ast::types::Return>());
    REQUIRE(ret->forward_edges().size() == 1);

    node_iter ret_val = ret->forward_edges().front()->target();
    REQUIRE(ret_val->get_data().is<frontend::ast::types::ID>());
    REQUIRE(ret_val->get_data().get<frontend::ast::types::ID>().name == "x");
}

TEST_CASE(
    "check_semantics annotates declared nodes with types", "[semantic][ast]"
) {
    const std::string src = "int32_t main() { int32_t x = 1; return x; }\n";
    frontend::parser::ParserContext ctx(src);
    REQUIRE(ctx.parse());
    REQUIRE(ctx.ast.check_semantics(ctx));

    bool found_decl = false;
    bool found_id = false;

    visit_nodes(ctx.ast.get_root(), [&](node_iter node) {
        auto &data = node->get_data();
        if (data.is<frontend::ast::types::Declare>()
            && data.get<frontend::ast::types::Declare>().name == "x") {
            found_decl = true;
            REQUIRE(data.type.has_value());
            REQUIRE(data.type->repr(ctx.ast.types) == "int32_t");
        }
        if (data.is<frontend::ast::types::ID>()
            && data.get<frontend::ast::types::ID>().name == "x") {
            found_id = true;
            REQUIRE(data.type.has_value());
            REQUIRE(data.type->repr(ctx.ast.types) == "int32_t");
        }
    });

    REQUIRE(found_decl);
    REQUIRE(found_id);
}
