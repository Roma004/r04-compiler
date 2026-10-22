#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "type_test_utils.hpp"

using namespace type_test;

TEST_CASE("expression type checking", "[semantic][types]") {
    struct case_t {
        const char *source;
        const char *expected_error;
    };

    const std::vector<case_t> cases = {
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t *p = &x;
    int32_t y = p + 1;
    return 0;
}
)",
            "invalid operands to binary operator `+`",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    int32_t y = p + 1;
    return 0;
}
)",
            "invalid operands to binary operator `+`",
        },
        {
            R"(
void_t noop();
int32_t main() {
    void_t v = noop();
    int32_t y = v + 1;
    return 0;
}
)",
            "invalid operands to binary operator `+`",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t *p = &x;
    int32_t y = -p;
    return 0;
}
)",
            "invalid operand to unary operator `-`",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    int32_t y = ~p;
    return 0;
}
)",
            "invalid operand to unary operator `~`",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t y = x[0];
    return 0;
}
)",
            "subscripted value is not an array or a pointer",
        },
        {
            R"(
int32_t main() {
    int32_t[4] arr;
    int32_t *p = &arr[0];
    int32_t y = arr[p];
    return 0;
}
)",
            "subscript index is not an integer",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t y = *x;
    return 0;
}
)",
            "cannot dereference a non-pointer value",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    int32_t y = p->x;
    return 0;
}
)",
            "cannot dereference a non-pointer value",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    int32_t y = x.field;
    return 0;
}
)",
            "member access on a non-struct value",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    point_t *pp = &p;
    int32_t y = pp.x;
    return 0;
}
)",
            "member access on a non-struct value",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    int32_t y = p.z;
    return 0;
}
)",
            "no member named `z`",
        },
        {
            R"(
struct point_t { int32_t x; }
int32_t main() {
    point_t p;
    point_t *pp = &p;
    int32_t y = pp->z;
    return 0;
}
)",
            "no member named `z`",
        },
        {
            R"(
int32_t main() {
    int32_t x = 1;
    x();
    return 0;
}
)",
            "unknown function `x`",
        },
    };

    for (const auto &test : cases)
        check_invalid(test.source, test.expected_error);
}
