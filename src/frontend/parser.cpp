#include <ctpg/ctpg.hpp>
#include <iostream>

#include "frontend/ast/ast.hpp"
#include "frontend/ast/types.hpp"
#include "frontend/parser.hpp"
#include "tools/macro_template.hpp"

frontend::ast::types::Literal::Literal(std::string_view sv) {
    // 1. Определяем основание и сдвигаем начало цифр
    std::string_view digits = sv;
    int base = 10;
    if (digits.size() > 2 && digits[0] == '0'
        && (digits[1] == 'x' || digits[1] == 'X')) {
        base = 16;
        digits.remove_prefix(2);
    } else if (
        digits.size() > 2 && digits[0] == '0'
        && (digits[1] == 'b' || digits[1] == 'B')
    ) {
        base = 2;
        digits.remove_prefix(2);
    } else if (digits.size() > 1 && digits[0] == '0') {
        base = 8;
        digits.remove_prefix(1);
    }

    // 2. Отделяем суффикс (u/U, l/L, ll/LL в любом порядке)
    size_t suffix_start = digits.find_first_not_of("0123456789abcdefABCDEF");
    std::string_view suffix;
    if (suffix_start != std::string_view::npos) {
        suffix = digits.substr(suffix_start);
        digits = digits.substr(0, suffix_start);
    }

    // 3. Парсим значение
    uint64_t val = 0;
    auto [ptr, ec] = std::from_chars(
        digits.data(), digits.data() + digits.size(), val, base
    );
    if (ec != std::errc{}) throw std::runtime_error("invalid integer literal");

    // 4. Анализ суффикса
    bool has_u = false, has_l = false, has_ll = false;
    for (char c : suffix) {
        if (c == 'u' || c == 'U') has_u = true;
        else if (c == 'l' || c == 'L') {
            if (has_l) has_ll = true;
            else has_l = true;
        }
    }

    // 5. Определяем размер и знаковость
    //    В C int = 4 байта, long/long long = 8 байт (на 64-битных платформах)
    size = has_l ? 8 : 4;
    is_signed = !has_u;

    value = val & mask_for_size(size);
}

uint64_t frontend::ast::types::Literal::mask_for_size(uint8_t sz) {
    if (sz >= 8) return ~0ULL;
    return (1ULL << (sz * 8)) - 1ULL;
}

#define DECL_KEYWORD(name, str) constexpr string_term term_kwd_##name(str)
#define DECL_TYPE(name, str)    constexpr string_term term_type_##name(str)
#define DECL_BINARY_OP(name, str, order, assoc)  \
    constexpr string_term term_op_binary_##name( \
        str, order, associativity::assoc         \
    )
#define DECL_UNARY_OP(name, str) constexpr string_term term_op_unary_##name(str)
#define DECL_SIGN_ASSOC(name, str, order, assoc) \
    constexpr string_term term_sign_##name(str, order, associativity::assoc)
#define DECL_SIGN(name, str, order) \
    constexpr string_term term_sign_##name(str, order)

#define KEYWORDS(...)   FOR_EACH(term::term_kwd_, , __VA_ARGS__)
#define TYPES(...)      FOR_EACH(term::term_type_, , __VA_ARGS__)
#define BINARY_OPS(...) FOR_EACH(term::term_op_binary_, , __VA_ARGS__)
#define UNARY_OPS(...)  FOR_EACH(term::term_op_unary_, , __VA_ARGS__)
#define SIGNS_OPS(...)  FOR_EACH(term::term_sign_, , __VA_ARGS__)

#define KWD(name) term::term_kwd_##name
#define TYP(name) term::term_type_##name
#define BIN(name) term::term_op_binary_##name
#define UNR(name) term::term_op_unary_##name
#define SGN(name) term::term_sign_##name

namespace frontend::parser {

using namespace ctpg;
using namespace ctpg::buffers;
using namespace ctpg::ftors;

namespace term {
constexpr char id_pattern[] = "[A-Za-z_][0-9A-Za-z_]*";
constexpr regex_term<id_pattern> id("id");

constexpr char num_pattern[] =
    "[1-9][0-9]*|0[0-7]*|0[xX][0-9a-fA-F]+|0[bB][01]+";
constexpr regex_term<num_pattern> num_literal("dec_literal");

DECL_KEYWORD(if, "if");
DECL_KEYWORD(else, "else");
DECL_KEYWORD(while, "while");
DECL_KEYWORD(return, "return");
DECL_KEYWORD(const, "const");
DECL_KEYWORD(volatile, "volatile");
DECL_KEYWORD(signed, "signed");

DECL_TYPE(void, "void");
DECL_TYPE(char, "char");
DECL_TYPE(int, "int");
DECL_TYPE(bool, "bool");

enum prior {
    ASSIGN,
    LOR,
    LAND,
    BOR,
    BXOR,
    BAND,
    EQ,
    CMP,
    SHIFT,
    MATH0,
    MATH1,
    UNARY,
    DEREF,
    EDGE
};

DECL_BINARY_OP(assign, "=", prior::ASSIGN, rtol);
DECL_BINARY_OP(lor, "||", prior::LOR, ltor);
DECL_BINARY_OP(land, "&&", prior::LAND, ltor);
DECL_BINARY_OP(bor, "|", prior::BOR, ltor);
DECL_BINARY_OP(bxor, "^", prior::BXOR, ltor);
DECL_BINARY_OP(band, "&", prior::BAND, ltor);
DECL_BINARY_OP(eq, "==", prior::EQ, ltor);
DECL_BINARY_OP(ne, "!=", prior::EQ, ltor);
DECL_BINARY_OP(lt, "<", prior::CMP, ltor);
DECL_BINARY_OP(gt, ">", prior::CMP, ltor);
DECL_BINARY_OP(le, "<=", prior::CMP, ltor);
DECL_BINARY_OP(ge, ">=", prior::CMP, ltor);
DECL_BINARY_OP(rsh, ">>", prior::SHIFT, ltor);
DECL_BINARY_OP(lsh, "<<", prior::SHIFT, ltor);
DECL_BINARY_OP(add, "+", prior::MATH0, ltor);
DECL_BINARY_OP(sub, "-", prior::MATH0, ltor);
DECL_BINARY_OP(mul, "*", prior::MATH1, ltor);
DECL_BINARY_OP(div, "/", prior::MATH1, ltor);
DECL_BINARY_OP(mod, "%", prior::MATH1, ltor);

DECL_UNARY_OP(neg, "-");
DECL_UNARY_OP(log_not, "!");
DECL_UNARY_OP(bin_not, "~");

DECL_SIGN(deref, "@", prior::DEREF);
DECL_SIGN(addr, "&", prior::DEREF);
DECL_SIGN_ASSOC(edge, "->", prior::EDGE, ltor);
DECL_SIGN_ASSOC(dot, ".", prior::EDGE, ltor);

}; // namespace term

using decl_t = ast::Ast::node_iter;
using rval_t = ast::Ast::node_iter;
using lval_t = ast::Ast::node_iter;

constexpr nterm<decl_t> program("program");
constexpr nterm<rval_t> rval("rval");
constexpr nterm<lval_t> lval("lval");

#define node_from_sv(ctx, type, sv)                                        \
    ctx.ast.emplace_node(                                                  \
        ast::node_t::from_sv<ast::types::type>(sv, ctx.src_map.locate(sv)) \
    );

#define node_from_svloc(ctx, type, sv)                                 \
    ctx.ast.emplace_node(                                              \
        ast::node_t::from_loc<ast::types::type>(ctx.src_map.locate(sv)) \
    );

#define node_from_loc(ctx, type, loc) \
    ctx.ast.emplace_node(ast::node_t::from_loc<ast::types::type>(loc));

// clang-format off
constexpr ctpg::parser p(program,
terms(
    term::id,
    term::num_literal,
    KEYWORDS(const, volatile, if, else, while, return, signed),
    TYPES(void, char, int, bool),
    BINARY_OPS(
        assign, lor, land, bor, band, bxor, rsh, lsh, eq, ne, lt, gt, le, ge,
        add, sub, mul, div, mod
    ),
    UNARY_OPS(neg, log_not, bin_not),
    SIGNS_OPS(deref, addr, edge, dot),
    '(', ')', '[', ']', '{', '}', ';', ','
),
nterms(program, rval, lval),
rules(
    program() >>=
        [](ParserContext &ctx) {
            auto root = ctx.ast.emplace_node(ast::types::Root{});
            ctx.ast.set_root(root);
            return root;
        },
    program(program, lval) >>=
        [](ParserContext &ctx, decl_t &&root, lval_t &&decl) {
            ctx.ast.emplace_edge(root, decl, root->forward_edges().size());
            return root;
        },

    rval(term::num_literal) >>=
        [](ParserContext &ctx, std::string_view sv) {
            return node_from_sv(ctx, Literal, sv);
        },
    rval(SGN(addr), lval) >>=
        [](ParserContext &ctx, std::string_view sv, lval_t &&val) {
            auto res = node_from_svloc(ctx, Addr, sv);
            ctx.ast.emplace_edge(res, val, 0);
            return res;
        },
    rval(lval) >= _e1,
    // rval('(', rval, ')') >= _e2,
    // TODO: operations rval = rval _op_ rval
    // TODO: operations rval = _op_ rval

    lval(term::id) >>=
        [](ParserContext &ctx, std::string_view id) {
            return node_from_sv(ctx, ID, id);
        },
    lval(lval, '[', rval, ']') >>=
        [](ParserContext &ctx, lval_t lv, skip, rval_t rv, skip) {
            auto res = node_from_loc(ctx, Subscript, lv->get_data().loc);
            ctx.ast.emplace_edge(res, lv, 0);
            ctx.ast.emplace_edge(res, rv, 1);
            return res;
        },
    lval(SGN(deref), rval) >>=
        [](ParserContext &ctx, std::string_view sv, rval_t rv) {
            auto res = node_from_svloc(ctx, Deref, sv);
            ctx.ast.emplace_edge(res, rv, 1);
            return res;
        },
    lval(lval, SGN(dot), lval) >>=
        [](ParserContext &ctx, lval_t base, std::string_view sv, lval_t member) {
            auto res = node_from_sv(ctx, Get, sv);
            ctx.ast.emplace_edge(res, base, 0);
            ctx.ast.emplace_edge(res, member, 1);
            return res;
        },
    lval(lval, SGN(edge), lval) >>=
        [](ParserContext &ctx, lval_t base, std::string_view sv, lval_t member) {
            auto res = node_from_sv(ctx, Get, sv);
            auto deref = node_from_svloc(ctx, Deref, sv);
            ctx.ast.emplace_edge(deref, base, 0);
            ctx.ast.emplace_edge(res, deref, 0);
            ctx.ast.emplace_edge(res, member, 1);
            return res;
        }
));
// clang-format on

bool ParserContext::parse() {

    auto res = p.context_parse(
        *this,
        parse_options{}.set_verbose(),
        string_buffer(src_map.source.data()),
        std::cerr
    );
    return res.has_value();
}

}; // namespace frontend::parser

// clang-format off
#define BINARY_FOR(res_t, l_t, _op_, r_t)                                 \
    res_t(l_t, #_op_, r_t) >>=                                            \
        [](ParserContext &ctx, auto &&l, std::string_view op, auto &&r) { \
            try {                                                         \
                return l _op_ r;                                          \
            } catch (const InvalidOperation &e) {                         \
                ctx.insert_error(                                         \
                    std::string("invalid operation: ") + e.what(), op     \
                );                                                        \
            }                                                             \
        }
#define UNARY_FOR(res_t, _op_, r_t)                                   \
    res_t(#_op_, r_t) >>=                                             \
        [](ParserContext &ctx, std::string_view op, auto &&r) {       \
            try {                                                     \
                return _op_ r;                                        \
            } catch (const InvalidOperation &e) {                     \
                ctx.insert_error(                                     \
                    std::string("invalid operation: ") + e.what(), op \
                );                                                    \
            }                                                         \
        }

// clang-format off
#define MATH_OPERATORS(res_t, l_t, r_t) \
    BINARY_FOR(res_t, l_t, +, r_t),     \
    BINARY_FOR(res_t, l_t, -, r_t),     \
    BINARY_FOR(res_t, l_t, *, r_t),     \
    BINARY_FOR(res_t, l_t, /, r_t),     \
    BINARY_FOR(res_t, l_t, %, r_t),     \
    UNARY_FOR(res_t, -, l_t)

#define BITWISE_OPERATORS(res_t, l_t, r_t) \
    BINARY_FOR(res_t, l_t, &, r_t),      \
    BINARY_FOR(res_t, l_t, |, r_t),      \
    BINARY_FOR(res_t, l_t, ^, r_t),      \
    BINARY_FOR(res_t, l_t, <<, r_t),     \
    BINARY_FOR(res_t, l_t, >>, r_t),     \
    UNARY_FOR(res_t, ~, l_t)

#define LOGIC_OPERATORS(res_t, l_t, r_t) \
    BINARY_FOR(res_t, l_t, &&, r_t),     \
    BINARY_FOR(res_t, l_t, ||, r_t),     \
    UNARY_FOR(res_t, !, l_t)

#define COMPARE_OPERATORS(res_t, l_t, r_t) \
    BINARY_FOR(res_t, l_t, ==, r_t),       \
    BINARY_FOR(res_t, l_t, !=, r_t),       \
    BINARY_FOR(res_t, l_t, <, r_t),        \
    BINARY_FOR(res_t, l_t, >, r_t),        \
    BINARY_FOR(res_t, l_t, <=, r_t),       \
    BINARY_FOR(res_t, l_t, >=, r_t)

