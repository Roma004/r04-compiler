#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "type_test_utils.hpp"

using namespace type_test;

TEST_CASE(
    "type modifiers are applied from right to left", "[semantic][types]"
) {
    struct case_t {
        const char *decl;
        const char *name;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"uint32_t v;", "v", "uint32_t"},
        {"uint32_t *p;", "p", "uint32_t *"},
        {"uint32_t **pp;", "pp", "uint32_t * *"},
        {"uint32_t[3] a;", "a", "uint32_t [3]"},
        {"uint32_t[2][3] m;", "m", "uint32_t [2] [3]"},
        {"uint32_t *[3] ap;", "ap", "uint32_t * [3]"},
        {"uint32_t[3] *pa;", "pa", "uint32_t [3] *"},
        {"uint32_t *[2][3] *ppa;", "ppa", "uint32_t * [2] [3] *"},
    };

    for (const auto &test : cases)
        REQUIRE(decl_type_in(test.decl, test.name) == test.expected);
}

TEST_CASE(
    "const qualifies the type construct on its right", "[semantic][types]"
) {
    struct case_t {
        const char *decl;
        const char *name;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"const uint32_t v;", "v", "uint32_t const"},
        {"uint32_t const v;", "v", "uint32_t const"},
        {"const uint32_t *p;", "p", "uint32_t const *"},
        {"uint32_t const *p;", "p", "uint32_t * const"},
        {"uint32_t *const p;", "p", "uint32_t * const"},
        {"const uint32_t *const p;", "p", "uint32_t const * const"},
        {"const uint32_t[10] a;", "a", "uint32_t const [10]"},
        {"uint32_t const [10] a;", "a", "uint32_t [10] const"},
        {"const uint32_t const v;", "v", "uint32_t const"},
        {"const uint32_t *[5]const[3] a;",
         "a",
         "uint32_t const * [5] [3] const"},
        {"const uint32_t [5][10] *a;", "a", "uint32_t const [5] [10] *"},
        {"uint32_t const *[5][3] a;", "a", "uint32_t * const [5] [3]"},
    };

    for (const auto &test : cases)
        REQUIRE(decl_type_in(test.decl, test.name) == test.expected);
}

TEST_CASE("subscripts consume modifiers from the right", "[semantic][types]") {
    const char *decls = R"(
const uint32_t *[5]const[3] capp;
const uint32_t [5][10] *cmat;
uint32_t[2][3] matrix;
)";

    REQUIRE(expr_type_in(decls, "capp[1]") == "uint32_t const * [5]");
    REQUIRE(expr_type_in(decls, "capp[1][2]") == "uint32_t const *");
    REQUIRE(expr_type_in(decls, "capp[1][2][3]") == "uint32_t const");
    REQUIRE(expr_type_in(decls, "cmat[3]") == "uint32_t const [5] [10]");
    REQUIRE(expr_type_in(decls, "cmat[3][9]") == "uint32_t const [5]");
    REQUIRE(expr_type_in(decls, "cmat[3][9][4]") == "uint32_t const");
    REQUIRE(expr_type_in(decls, "matrix[0]") == "uint32_t [2]");
    REQUIRE(expr_type_in(decls, "matrix[0][1]") == "uint32_t");
}

TEST_CASE("const and array assignment targets", "[semantic][types]") {
    struct case_t {
        const char *src;
        const char *expected;
    };

    const std::vector<case_t> invalid = {
        {"void_t main() { const int32_t x = 1; x = 2; }",
         "cannot assign to a const value"},
        {"void_t main() { const int32_t x = 1; x += 2; }",
         "cannot assign to a const value"},
        {"void_t main() { const int32_t x = 1; const int32_t *p = &x; *p = 2; "
         "}",
         "cannot assign to a const value"},
        {"void_t main() { int32_t x = 1; int32_t const *p = &x; p = &x; }",
         "cannot assign to a const value"},
        {"void_t main() { const int32_t[4] a; a[0] = 1; }",
         "cannot assign to a const value"},
        {"void_t main() { int32_t[4] a; a = a; }", "cannot assign to an array"},
        {"void_t main() { int32_t const [4] a; a = a; }",
         "cannot assign to an array"},
        {"void_t main() { const int32_t[4] a; a = a; }",
         "cannot assign to an array"},
    };

    for (const auto &test : invalid) check_invalid(test.src, test.expected);

    const std::vector<const char *> valid = {
        "void_t main() { const int32_t x = 1; int32_t y = x; }",
        "void_t main() { const int32_t x = 1; const int32_t *p = &x; p = &x; }",
        "void_t main() { int32_t x = 1; int32_t const *p = &x; *p = 2; }",
        "void_t main() { int32_t[4] a; a[0] = 1; }",
        "void_t main() { int32_t[4] a; a[0] += 1; }",
        "void_t main() { int32_t const [4] a; a[0] = 1; }",
        "void_t main() { const uint32_t[4] a; uint32_t x = a[0]; }",
    };

    for (const char *src : valid) check_valid(src);
}
