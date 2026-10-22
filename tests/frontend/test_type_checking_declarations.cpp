#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "type_test_utils.hpp"

using namespace type_test;

TEST_CASE("declaration type checking", "[semantic][types]") {
    struct case_t {
        const char *source;
        const char *expected_error;
    };

    const std::vector<case_t> cases = {
        {
            R"(
struct point_t { int32_t x; }
struct point_t { int32_t y; }
int32_t main() { return 0; }
)",
            "redefinition of type point_t",
        },
        {
            R"(
struct node_t;
struct tree_t { node_t value; }
int32_t main() { return 0; }
)",
            "type of field value is not fully defined",
        },
        {
            R"(
struct point_t { int32_t x; int32_t x; }
int32_t main() { return 0; }
)",
            "field with such name already defined",
        },
        {
            R"(
int32_t main() {
    int32_t n = 3;
    int32_t[n] arr;
    return 0;
}
)",
            "Got non-constant expression in constant context",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t y = (foo_t) x;
    return 0;
}
)",
            "type foo_t is not defined",
        },
        {
            R"(
struct point_t { foo_t x; }
int32_t main() { return 0; }
)",
            "type foo_t is not defined",
        },
    };

    for (const auto &test : cases)
        check_invalid(test.source, test.expected_error);
}
