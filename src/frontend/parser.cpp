#include "frontend/parser.hpp"
#include "ctpg/parse-table.hpp"
#include "frontend/ast/source_map.hpp"
#include "frontend/ast/types.hpp"
#include <ctpg/ctpg.hpp>
#include <iostream>
#include <optional>

#ifndef CTPG_TABLE_FILE
#define CTPG_TABLE_FILE "frontend_parser_table_data.bin"
#endif
#include <ctpg/parse-table/mapped.hpp>

#define DECL_REGEX_TERM(_name, regex_str)         \
    constexpr char _name##_pattern[] = regex_str; \
    ctpg::regex_term<_name##_pattern> _name(#_name);

#define DECL_KEYWORD(name) constexpr ctpg::string_term term_kwd_##name(#name)
#define DECL_SIGN_ASSOC(name, str, order, assoc)  \
    constexpr ctpg::string_term term_sign_##name( \
        str, order, ctpg::associativity::assoc    \
    )
#define DECL_SIGN(name, str, order) \
    constexpr ctpg::string_term term_sign_##name(str, order)

#define DECL_NTERM(name, type) constexpr ctpg::nterm<type> name(#name)
#define DECL_NODE_NTERM(name)  DECL_NTERM(name, node);
#define DECL_LIST_NTERM(name)  DECL_NTERM(name, list);

#define KEYWORDS(...)   FOR_EACH(term::term_kwd_, , __VA_ARGS__)
#define BINARY_OPS(...) FOR_EACH(term::term_op_binary_, , __VA_ARGS__)
#define UNARY_OPS(...)  FOR_EACH(term::term_op_unary_, , __VA_ARGS__)
#define SIGNS(...)      FOR_EACH(term::term_sign_, , __VA_ARGS__)

#define KWD(name) term::term_kwd_##name
#define BIN(name) term::term_op_binary_##name
#define UNR(name) term::term_op_unary_##name
#define SGN(name) term::term_sign_##name

// clang-format off
#define LIST_OF(nterm, of_nterm, sep) \
    nterm()                               >>= create<list>{}, \
    nterm(nterm##_ne)                      >=  _e1, \
    nterm##_ne(of_nterm)                   >= create_list, \
    nterm##_ne(nterm##_ne, sep, of_nterm)  >= push_back<1, 3>{}
// clang-format on

namespace frontend::parser {

using namespace ctpg::ftors;
using namespace ast;

namespace term {

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


DECL_REGEX_TERM(str_literal, R"regex("([^"\\]|\\.)*")regex");
DECL_REGEX_TERM(chr_literal, R"regex('([^'\\]|\\.)*')regex");
DECL_REGEX_TERM(id, "[A-Za-z_][0-9A-Za-z_]*");
DECL_REGEX_TERM(type_id, "[A-Za-z_][0-9A-Za-z_]*_t");
DECL_REGEX_TERM(
    num_literal, "[1-9][0-9]*|0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*"
);

DECL_KEYWORD(if);
DECL_KEYWORD(else);
DECL_KEYWORD(for);
DECL_KEYWORD(return);
DECL_KEYWORD(const);
DECL_KEYWORD(volatile);
DECL_KEYWORD(break);
DECL_KEYWORD(continue);

DECL_SIGN_ASSOC(assign, "=", prior::ASSIGN, rtol);
DECL_SIGN_ASSOC(deref, "*", prior::DEREF, ltor);
DECL_SIGN_ASSOC(addr, "&", prior::DEREF, ltor);
DECL_SIGN_ASSOC(edge, "->", prior::EDGE, ltor);
DECL_SIGN_ASSOC(dot, ".", prior::EDGE, ltor);

}; // namespace term

DECL_NODE_NTERM(program);
DECL_NODE_NTERM(global_stmt);
DECL_NODE_NTERM(stmt);
DECL_NODE_NTERM(block);
DECL_NODE_NTERM(expr);
DECL_NODE_NTERM(expr_assign);
DECL_NODE_NTERM(type_spec);
DECL_NODE_NTERM(decl);
DECL_NODE_NTERM(decl_assign);
DECL_NODE_NTERM(decl_function);
DECL_NTERM(for_cond, std::optional<node>);

DECL_LIST_NTERM(call_args);
DECL_LIST_NTERM(call_args_ne);
DECL_LIST_NTERM(decl_list);
DECL_LIST_NTERM(decl_list_ne);
DECL_LIST_NTERM(stmt_list);
DECL_LIST_NTERM(for_init);
DECL_LIST_NTERM(for_init_ne);
DECL_LIST_NTERM(for_step);
DECL_LIST_NTERM(for_step_ne);


// clang-format off
ctpg::parser p(program,
terms(
    KEYWORDS(const, volatile, if, else, for, return, break, continue),
    term::chr_literal,
    term::str_literal,
    term::num_literal,
    term::type_id,
    term::id,
    SIGNS(assign, deref, addr, edge, dot),
    "||", "&&", "|", "^", "&", "==", "!=", "<", ">", "<=", ">=", ">>", "<<",
    "+", "-", "*", "/", "%", "~", "!", "|=", "^=", "&=", ">>=", "<<=", "+=",
    "-=", "*=", "/=", "%=",
    '(', ')', '[', ']', "{", '}', ';', ','
),
nterms(
    program, global_stmt, expr, type_spec, decl, decl_list, decl_list_ne,
    decl_function, decl_assign, block, stmt_list, stmt, call_args, expr_assign,
    call_args_ne, for_init, for_init_ne, for_cond, for_step, for_step_ne
),
rules(
    /** program is list of global statements */
    program()                     >>= make_root,
    program(program, global_stmt) >>= add_global_stmt,

    /**
     * global statement might be:
     * - declaration
     * - declaration with assignment
     * - declaration of function
     * - declaration of function with body
     */
    global_stmt(decl_assign, ';')      >= _e1,
    global_stmt(decl_function, ';')    >= _e1,
    global_stmt(decl_function, block) >>= connect_with_body,

    /**
     * `decl` is a declaration of name preceeded with type
     */
    decl(type_spec, term::id) >>= make_decl,

    /**
     * `decl_wit_def` is a declaration of name preceeded with type with default
     * value providen or not.
     */
    decl_assign(decl)                     >= _e1,
    decl_assign(decl, SGN(assign), expr) >>= make_assign,

    /**
     * `decl_function` is a statement like __type__ ID(args...)
     */
    decl_function(decl, '(',  decl_list, ')') >>= make_fn_decl,

    /**
     * `block` is a list of statements wrapped into '{', '}'
     */
    block("{", stmt_list, '}') >>= make_block,

    /**
     * `stmt_list` is a sequence of local context stetements
     */
    stmt_list()                 >= create<list>{},
    stmt_list(stmt_list, stmt)  >= push_back<1, 2>{},

    /**
     * `stmt` is any statement in local context. it might be:
     * - expression
     * - declaration
     * - declaration with default value
     * - assignment of two expressions
     * - block
     * - conditional (if-else) statement
     * - cycle
     * - return
     */
    stmt(block)                   >= _e1,
    stmt(decl_assign, ';')        >= _e1,
    stmt(expr_assign, ';')        >= _e1,
    stmt(KWD(return), expr, ';') >>= make_return,
    stmt(KWD(return), ';')       >>= create_kwd<types::Return>,
    stmt(KWD(break), ';')        >>= create_kwd<types::Break>,
    stmt(KWD(continue), ';')     >>= create_kwd<types::Continue>,

    stmt(KWD(if), '(', expr, ')', stmt)                  >>= make_if,
    stmt(KWD(if), '(', expr, ')', stmt, KWD(else), stmt) >>= make_if_else,
    stmt(KWD(for), '(', for_init, ';', for_cond, ';', for_step, ')', stmt)
                                                         >>= make_for,

    for_cond()     >= [](){ return std::nullopt; },
    for_cond(expr) >= _e1,

    /**
     * `type_spec` is any id, qualified using const and volatile specifiers.
     * All subscript and dereference operators are treated as array or
     * pointer dimensions of that type.
     */
    type_spec(term::type_id)             >>= create_from_sv<types::TypeSpec>,
    type_spec(KWD(const), type_spec)     >>= add_const,
    type_spec(KWD(volatile), type_spec)  >>= add_volatile,
    type_spec(type_spec, SGN(deref))     >>= add_deref,
    type_spec(type_spec, '[', expr, ']') >>= add_subscript,

    /**
     * Expression is one of:
     * - numeric literal
     * - in-language name
     * - address of expression
     * - dereference of expression
     * - subscription of expression
     * - any member name of expression
     * - cast of expression into another type
     * - any binary operation between expressions
     * - any unary operation to the expressions
     * - call of the function
     * - expression inside parenthness
     */
    expr(term::num_literal)             >>= create_from_sv<types::Literal>,
    expr(term::chr_literal)             >>= create_from_sv<types::CharLiteral>,
    expr(term::str_literal)             >>= create_from_sv<types::StringLiteral>,
    expr(term::id)                      >>= create_from_sv<types::ID>,
    expr(SGN(addr), expr)               >>= make_address,
    expr(SGN(deref), expr)              >>= make_dereference,
    expr(expr, '[', expr, ']')          >>= make_subscript,
    expr(expr, SGN(dot), term::id)      >>= make_dot_member,
    expr(expr, SGN(edge), term::id)     >>= make_edge_member,
    expr('(', type_spec, ')', expr)     >>= make_cast,
    expr('(', expr, ')')                 >= _e2,
    expr(expr, '(', call_args, ')')     >>= make_call,
    expr("-", expr)[term::prior::UNARY] >>= make_unary_op,
    expr("~", expr)[term::prior::UNARY] >>= make_unary_op,
    expr("!", expr)[term::prior::UNARY] >>= make_unary_op,
    expr(expr, "||", expr)[term::prior::LOR  ] >>= make_binary_op,
    expr(expr, "&&", expr)[term::prior::LAND ] >>= make_binary_op,
    expr(expr, "|",  expr)[term::prior::BOR  ] >>= make_binary_op,
    expr(expr, "^",  expr)[term::prior::BXOR ] >>= make_binary_op,
    expr(expr, "&",  expr)[term::prior::BAND ] >>= make_binary_op,
    expr(expr, "==", expr)[term::prior::EQ   ] >>= make_binary_op,
    expr(expr, "!=", expr)[term::prior::EQ   ] >>= make_binary_op,
    expr(expr, "<",  expr)[term::prior::CMP  ] >>= make_binary_op,
    expr(expr, ">",  expr)[term::prior::CMP  ] >>= make_binary_op,
    expr(expr, "<=", expr)[term::prior::CMP  ] >>= make_binary_op,
    expr(expr, ">=", expr)[term::prior::CMP  ] >>= make_binary_op,
    expr(expr, ">>", expr)[term::prior::SHIFT] >>= make_binary_op,
    expr(expr, "<<", expr)[term::prior::SHIFT] >>= make_binary_op,
    expr(expr, "+",  expr)[term::prior::MATH0] >>= make_binary_op,
    expr(expr, "-",  expr)[term::prior::MATH0] >>= make_binary_op,
    expr(expr, "*",  expr)[term::prior::MATH1] >>= make_binary_op,
    expr(expr, "/",  expr)[term::prior::MATH1] >>= make_binary_op,
    expr(expr, "%",  expr)[term::prior::MATH1] >>= make_binary_op,
    expr(expr, "|=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "^=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "&=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, ">>=", expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "<<=", expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "+=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "-=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "*=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "/=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,
    expr(expr, "%=",  expr)[term::prior::ASSIGN] >>= make_binary_assign,

    /**
     * `decl_assign` is ether an expr or assignment of two exprs.
     */
    expr_assign(expr)                     >= _e1,
    expr_assign(expr, SGN(assign), expr) >>= make_assign,

    /**
     * `decl_list` is a list of declarations separated by ',', which may be
     * a sequence of function arguments
     */
    LIST_OF(decl_list, decl, ','),

    /**
     * `call_args` is a list of non-typed expression separated by `,`
     */
    LIST_OF(call_args, expr, ','),

    /**
     * `for_init` is a list of declarations whick may have default values
     */
    LIST_OF(for_init, decl_assign, ','),

    /**
     * `for_step` is a list of expressions
     */
    LIST_OF(for_step, expr_assign, ',')
));
// clang-format on

ctpg::parse_table &get_parse_table() noexcept {
    static auto table = p.create_table();
    return *table;
}

bool ParserContext::parse() {
    auto res = p.context_parse(
        *this,
        ctpg::make_parse_table_view(),
        ctpg::parse_options{},
        ctpg::buffers::string_view_buffer(src_map.source.data()),
        std::cerr
    );
    return res.has_value();
}

}; // namespace frontend::parser
