#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "type_test_utils.hpp"

using namespace type_test;

TEST_CASE("shift type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"i8 << 1", "int32_t"},
        {"u16 << 1", "int32_t"},
        {"u32 << 1", "uint32_t"},
        {"i64 >> 1", "int64_t"},
        {"u32 << u64", "uint32_t"},
        {"i8 >> i64", "int32_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("comparison and logical type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"i32 < u64", "int32_t"},
        {"i32 == i32", "int32_t"},
        {"i32 != u8", "int32_t"},
        {"i32 <= i64", "int32_t"},
        {"i32 > i32", "int32_t"},
        {"i32 >= u32", "int32_t"},
        {"i32 && pi", "int32_t"},
        {"i32 || flag", "int32_t"},
        {"!i32", "int32_t"},
        {"!pi", "int32_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("unary type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"-i8", "int32_t"},
        {"-i16", "int32_t"},
        {"-i32", "int32_t"},
        {"-i64", "int64_t"},
        {"-u8", "int32_t"},
        {"-u16", "int32_t"},
        {"-u32", "uint32_t"},
        {"-u64", "uint64_t"},
        {"-flag", "int32_t"},
        {"~i8", "int32_t"},
        {"~u16", "int32_t"},
        {"~u32", "uint32_t"},
        {"~u64", "uint64_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("cast type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"(int8_t) i32", "int8_t"},
        {"(uint64_t) i8", "uint64_t"},
        {"(bool_t) i32", "bool_t"},
        {"(int32_t *) pi", "int32_t *"},
        {"(const int32_t *) pi", "int32_t const *"},
        {"(int32_t * const) pi", "int32_t * const"},
        {"(int32_t[2]) arr", "int32_t [2]"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("subscript type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"arr[0]", "int32_t"},
        {"matrix[0]", "int32_t [2]"},
        {"matrix[0][1]", "int32_t"},
        {"matrix[1]", "int32_t [2]"},
        {"pi[2]", "int32_t"},
        {"(&arr)[0]", "int32_t [4]"},
        {"str[0]", "int8_t const"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("address and dereference type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"&i32", "int32_t *"},
        {"&u8", "uint8_t *"},
        {"&pt", "point_t *"},
        {"&pt.x", "int32_t *"},
        {"&arr[1]", "int32_t *"},
        {"&arr", "int32_t [4] *"},
        {"*(&i32)", "int32_t"},
        {"*pi", "int32_t"},
        {"*str", "int8_t const"},
        {"*ppt", "point_t"},
        {"*(&pt)", "point_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("member access type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"pt.x", "int32_t"},
        {"pt.inner", "inner_t"},
        {"pt.inner.a", "int32_t"},
        {"ppt->x", "int32_t"},
        {"ppt->inner", "inner_t"},
        {"ppt->inner.a", "int32_t"},
        {"(&pt)->x", "int32_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}

TEST_CASE("call type deduction", "[semantic][types]") {
    struct case_t {
        const char *expr;
        const char *expected;
    };

    const std::vector<case_t> cases = {
        {"id_fn()", "int32_t"},
        {"u64_fn()", "uint64_t"},
        {"noop(1)", "void_t"},
    };

    for (const auto &test : cases) check_expr_type(test.expr, test.expected);
}
