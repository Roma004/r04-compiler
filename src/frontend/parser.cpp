#include <ctpg/ctpg.hpp>
#include <iostream>

#include "frontend/ast/ast.hpp"
#include "frontend/ast/source_map.hpp"
#include "frontend/ast/types.hpp"
#include "frontend/parser.hpp"

#define DECL_KEYWORD(name, str) constexpr string_term term_kwd_##name(str)
#define DECL_BINARY_OP(name, str, order) \
    constexpr string_term term_op_binary_##name(str, order, associativity::ltor)
#define DECL_UNARY_OP(name, str) constexpr string_term term_op_unary_##name(str)
#define DECL_SIGN_ASSOC(name, str, order, assoc) \
    constexpr string_term term_sign_##name(str, order, associativity::assoc)
#define DECL_SIGN(name, str, order) \
    constexpr string_term term_sign_##name(str, order)

#define KEYWORDS(...)   FOR_EACH(term::term_kwd_, , __VA_ARGS__)
#define BINARY_OPS(...) FOR_EACH(term::term_op_binary_, , __VA_ARGS__)
#define UNARY_OPS(...)  FOR_EACH(term::term_op_unary_, , __VA_ARGS__)
#define SIGNS(...)      FOR_EACH(term::term_sign_, , __VA_ARGS__)

#define KWD(name) term::term_kwd_##name
#define BIN(name) term::term_op_binary_##name
#define UNR(name) term::term_op_unary_##name
#define SGN(name) term::term_sign_##name

#define sv2loc(sv) ctx.src_map.locate(sv)
#define create_node(type, ...) \
    ctx.ast.emplace_node(ast::node_t::create<ast::types::type>(__VA_ARGS__))

namespace frontend::parser {

using namespace ctpg;
using namespace ctpg::buffers;
using namespace ctpg::ftors;

using namespace ast;

namespace term {
constexpr char id_pattern[] = "[A-Za-z_][0-9A-Za-z_]*";
regex_term<id_pattern> id("id");

constexpr char type_id_pattern[] = "[A-Za-z_][0-9A-Za-z_]*_t";
regex_term<type_id_pattern> type_id("type_id");

constexpr char num_pattern[] =
    "[1-9][0-9]*|0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*";
regex_term<num_pattern> num_literal("num_literal");

DECL_KEYWORD(if, "if");
DECL_KEYWORD(else, "else");
DECL_KEYWORD(while, "while");
DECL_KEYWORD(return, "return");
DECL_KEYWORD(const, "const");
DECL_KEYWORD(volatile, "volatile");

enum prior {
    ASSIGN = 1,
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
    SUBSCRIPT,
    EDGE,
};

DECL_SIGN_ASSOC(assign, "=", prior::ASSIGN, rtol);
DECL_SIGN_ASSOC(deref, "*", prior::DEREF, ltor);
DECL_SIGN_ASSOC(addr, "&", prior::DEREF, ltor);
DECL_SIGN_ASSOC(edge, "->", prior::EDGE, ltor);
DECL_SIGN_ASSOC(dot, ".", prior::EDGE, ltor);

/**
 * Define binary operators this way to be able to declare this set of operators
 * for both runtime and compiletime expressions (which are dirrefent n-terms)
 * and explicity set them different priority
 */

// clang-format off
#define BINARY_OPERATIONS_FOR(nterm, __apply) \
    nterm(nterm, "||", nterm)[term::prior::LOR  ] __apply,  \
    nterm(nterm, "&&", nterm)[term::prior::LAND ] __apply,  \
    nterm(nterm, "|",  nterm)[term::prior::BOR  ] __apply,  \
    nterm(nterm, "^",  nterm)[term::prior::BXOR ] __apply,  \
    nterm(nterm, "&",  nterm)[term::prior::BAND ] __apply,  \
    nterm(nterm, "==", nterm)[term::prior::EQ   ] __apply,  \
    nterm(nterm, "!=", nterm)[term::prior::EQ   ] __apply,  \
    nterm(nterm, "<",  nterm)[term::prior::CMP  ] __apply,  \
    nterm(nterm, ">",  nterm)[term::prior::CMP  ] __apply,  \
    nterm(nterm, "<=", nterm)[term::prior::CMP  ] __apply,  \
    nterm(nterm, ">=", nterm)[term::prior::CMP  ] __apply,  \
    nterm(nterm, ">>", nterm)[term::prior::SHIFT] __apply,  \
    nterm(nterm, "<<", nterm)[term::prior::SHIFT] __apply,  \
    nterm(nterm, "+",  nterm)[term::prior::MATH0] __apply,  \
    nterm(nterm, "-",  nterm)[term::prior::MATH0] __apply,  \
    nterm(nterm, "*",  nterm)[term::prior::MATH1] __apply,  \
    nterm(nterm, "/",  nterm)[term::prior::MATH1] __apply,  \
    nterm(nterm, "%",  nterm)[term::prior::MATH1] __apply

#define UNARY_OPERATIONS_FOR(nterm, __apply) \
    nterm("-", nterm)[term::prior::UNARY] __apply,  \
    nterm("~", nterm)[term::prior::UNARY] __apply,  \
    nterm("!", nterm)[term::prior::UNARY] __apply
// clang-format on

}; // namespace term

using program_t = ast::Ast::node_iter;
using global_stmt_t = ast::Ast::node_iter;
using decl_t = ast::Ast::node_iter;
using expr_t = ast::Ast::node_iter;
using type_spec_t = ast::Ast::node_iter;

nterm<program_t> program("program");

/**
 * `global_stmt` is anything that could be placed in global context, such as
 * global variables, structures and functions
 */
nterm<global_stmt_t> global_stmt("global_stmt");

/**
 * `expr` is commnon non-terminal for both rvalue and lvalue expressions.
 * On this stage we do not distinguish them, as is is not yet a semantic
 * chek stage.
 */
nterm<expr_t> expr("expr");

/**
 * `type_spec` is generally a type of expression.
 *
 * E.G.: const volatile u32 [1, 2 + 5]@
 */
nterm<type_spec_t> type_spec("type_spec");

/**
 * `decl` is any declare statement (type_spec + id + [arrays...])
 */
nterm<decl_t> decl("decl");

expr_t binary_operation(
    ParserContext &ctx, expr_t l, std::string_view op, expr_t r
) {
    auto res = create_node(BinaryOp, sv2loc(op), op);
    ctx.ast.emplace_edge(res, l, 0);
    ctx.ast.emplace_edge(res, r, 1);
    return res;
}

expr_t unary_operation(ParserContext &ctx, std::string_view op, expr_t r) {
    auto res = create_node(UnaryOp, sv2loc(op), op);
    ctx.ast.emplace_edge(res, r, 0);
    return res;
}

// clang-format off
ctpg::parser p(program,
terms(
    KEYWORDS(const, volatile, if, else, while, return),
    term::num_literal,
    term::type_id,
    term::id,
    SIGNS(assign, deref, addr, edge, dot),
    "||", "&&", "|", "^", "&", "==", "!=", "<", ">", "<=", ">=", ">>", "<<",
    "+", "-", "*", "/", "%", "~", "!",
    '(', ')', '[', ']', '{', '}', ';', ','
),
nterms(program, global_stmt, expr, type_spec, decl),
rules(
    program() >>=
        [](ParserContext &ctx) {
            auto root = ctx.ast.emplace_node(node_t::create_root());
            ctx.ast.set_root(root);
            return root;
        },
    /** program is basically a list of global statements */
    program(program, global_stmt) >>=
        [](ParserContext &ctx, decl_t &&root, expr_t &&decl) {
            ctx.ast.emplace_edge(root, decl, root->forward_edges().size());
            return root;
        },


    /**
     * global statement might be:
     * - declaration
     * - declaration with assignment
     */
    global_stmt(decl, ';') >= _e1,
    global_stmt(decl, SGN(assign), expr, ';') >>=
        [](ParserContext &ctx, decl_t d, auto sv, expr_t e, skip) {
            auto res = create_node(Assign, sv2loc(sv));
            ctx.ast.emplace_edge(res, d, 0);
            ctx.ast.emplace_edge(res, e, 1);
            return res;
        },

    decl(type_spec, term::id) >>=
        [](ParserContext &ctx, type_spec_t type, auto id) {
            auto res = create_node(Declare, sv2loc(id), id);
            ctx.ast.emplace_edge(res, type, 0);
            return res;
        },

    /**
     * `type_spec` is any id, qualified using const and volatile specifiers.
     * All subscript and dereference operators are treated as array or
     * pointer dimensions of that type.
     */
    type_spec(term::type_id) >>=
        [](ParserContext &ctx, auto sv){
            return create_node(TypeSpec, sv2loc(sv), sv);
        },
    type_spec(KWD(const), type_spec) >=
        [](skip, type_spec_t type) {
            // TODO: if already add, insert warning
            type->get_data().get<types::TypeSpec>().add_const();
            return type;
        },
    type_spec(KWD(volatile), type_spec) >=
        [](skip, type_spec_t type) {
            // TODO: if already add, insert warning
            type->get_data().get<types::TypeSpec>().add_volatile();
            return type;
        },
    type_spec(type_spec, SGN(deref)) >>=
        [](ParserContext &ctx, type_spec_t type, auto sv) {
            auto child = create_node(Pointer, sv2loc(sv));
            ctx.ast.emplace_edge(type, child, type->forward_edges().size());
            return type;
        },
    type_spec(type_spec, '[', expr, ']') >>=
        [](ParserContext &ctx, type_spec_t type, skip, expr_t expr, skip) {
            ctx.ast.emplace_edge(type, expr, type->forward_edges().size());
            return type;
        },

    /**
     * `expr` itself may be built initialy as a numeric constant or as genral
     * identifier
     */
    expr(term::num_literal) >>=
        [](ParserContext &ctx, auto sv) {
            return create_node(Literal, sv2loc(sv), sv);
        },
    expr(term::id) >>=
        [](ParserContext &ctx, std::string_view id) {
            return create_node(ID, sv2loc(id), id);
        },

    /**
     * taking address or dereferencing of `expr` is another `expr`
     */
    expr(SGN(addr), expr) >>=
        [](ParserContext &ctx, std::string_view sv, expr_t &&val) {
            auto res = create_node(Addr, sv2loc(sv));
            ctx.ast.emplace_edge(res, val, 0);
            return res;
        },
    expr(SGN(deref), expr) >>=
        [](ParserContext &ctx, std::string_view sv, expr_t rv) {
            auto res = create_node(Deref, sv2loc(sv));
            ctx.ast.emplace_edge(res, rv, 1);
            return res;
        },

    /** Any expression may be subscripted by a value of another expression */
    expr(expr, '[', expr, ']') >>=
        [](ParserContext &ctx, expr_t lv, skip, expr_t rv, skip) {
            auto res = create_node(Subscript, lv->get_data().loc);
            ctx.ast.emplace_edge(res, lv, 0);
            ctx.ast.emplace_edge(res, rv, 1);
            return res;
        },
    /** Any expression may have members, so accept .id and ->id operators */
    expr(expr, SGN(dot), term::id) >>=
        [](ParserContext &ctx, expr_t base, skip, std::string_view id) {
            auto res = create_node(Get, sv2loc(id), id);
            ctx.ast.emplace_edge(res, base, 0);
            return res;
        },
    expr(expr, SGN(edge), term::id) >>=
        [](ParserContext &ctx, expr_t base, skip, std::string_view id) {
            auto res = create_node(Get, sv2loc(id), id);
            auto deref = create_node(Deref, sv2loc(id));
            ctx.ast.emplace_edge(deref, base, 0);
            ctx.ast.emplace_edge(res, deref, 1);
            return res;
        },

    /** Expression may be casted into any type */
    expr('(', type_spec, ')', expr) >>=
        [](ParserContext &ctx, auto sv, type_spec_t type, skip, expr_t expr) {
            auto res = create_node(Cast, type->get_data().loc);
            ctx.ast.emplace_edge(res, type, 0);
            ctx.ast.emplace_edge(res, expr, 1);
            return res;
        },

    /** rules for all binary and unary operations between expressions */
    BINARY_OPERATIONS_FOR(expr, >>= binary_operation),
    UNARY_OPERATIONS_FOR(expr, >>= unary_operation),

    /** Patenthness is allowed for expressions */
    expr('(', expr, ')') >= _e2
));
// clang-format on

bool ParserContext::parse() {
    auto table = p.create_table();

    auto res = p.context_parse(
        *this,
        table->view(),
        parse_options{}.set_verbose(),
        string_buffer(src_map.source.data()),
        std::cerr
    );
    return res.has_value();
}

}; // namespace frontend::parser
