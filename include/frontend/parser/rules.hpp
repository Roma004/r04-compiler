#pragma once

#include "frontend/ast/types.hpp"
#include <frontend/parser/context.hpp>

namespace frontend::parser {

using node = ast::Ast::node_iter;
using list = std::vector<node>;
using str = std::string_view;

#define CTX ParserContext &ctx

#define sv2loc(sv) ctx.src_map.locate(sv)

using namespace ast;

template <typename T, typename... Args>
inline node create_node(CTX, Args &&...args) {
    return ctx.ast.emplace_node(
        node_t::create<T>(std::forward<Args &&>(args)...)
    );
}

template <typename T> inline node create_from_sv(CTX, str sv) {
    return ctx.ast.emplace_node(node_t::create<T>(sv2loc(sv), sv));
}

template <typename T> inline node create_kwd(CTX, str sv, char) {
    return ctx.ast.emplace_node(node_t::create<T>(sv2loc(sv)));
}

inline std::vector<node> create_list(node n) { return std::vector<node>{n}; }

inline node make_root(CTX) {
    auto root = ctx.ast.emplace_node(ast::node_t::create_root());
    ctx.ast.set_root(root);
    return root;
}

inline node make_assign(CTX, node expr, str sv, node value) {
    auto res = create_node<types::Assign>(ctx, sv2loc(sv));
    ctx.ast.emplace_edge(res, expr, types::Assign::LEFT);
    ctx.ast.emplace_edge(res, value, types::Assign::RIGHT);
    return res;
}

inline node make_decl(CTX, node type, str id) {
    auto res = create_node<types::Declare>(ctx, sv2loc(id), id);
    ctx.ast.emplace_edge(res, type, 0);
    return res;
}

inline node make_fn_decl(CTX, node decl, char, list &&args, char) {
    auto res = create_node<types::Function>(ctx, decl->get_data().loc);
    ctx.ast.emplace_edge(res, decl, types::Function::DECL);
    int idx = types::Function::ARG0;
    for (auto arg : args) { ctx.ast.emplace_edge(res, arg, idx++); }
    return res;
}

inline node make_block(CTX, str sv, list &&lst, char) {
    auto res = create_node<types::Block>(ctx, sv2loc(sv));
    for (auto stmt : lst) {
        ctx.ast.emplace_edge(res, stmt, res->forward_edges().size());
    }
    return res;
}

inline node make_address(CTX, str sv, node value) {
    auto res = create_node<types::Addr>(ctx, sv2loc(sv));
    ctx.ast.emplace_edge(res, value, 0);
    return res;
}

inline node make_dereference(CTX, str sv, node value) {
    auto res = create_node<types::Deref>(ctx, sv2loc(sv));
    ctx.ast.emplace_edge(res, value, 0);
    return res;
}

inline node make_subscript(CTX, node lv, char, node rv, char) {
    auto res = create_node<types::Subscript>(ctx, lv->get_data().loc);
    ctx.ast.emplace_edge(res, lv, types::Subscript::VALUE);
    ctx.ast.emplace_edge(res, rv, types::Subscript::IDX);
    return res;
}

inline node make_edge_member(CTX, node base, str, str id) {
    auto res = create_node<types::Get>(ctx, sv2loc(id), id);
    auto deref = create_node<types::Deref>(ctx, sv2loc(id));
    ctx.ast.emplace_edge(deref, base, 0);
    ctx.ast.emplace_edge(res, deref, 1);
    return res;
}

inline node make_dot_member(CTX, node base, str, str id) {
    auto res = create_node<types::Get>(ctx, sv2loc(id), id);
    ctx.ast.emplace_edge(res, base, 0);
    return res;
}

inline node make_cast(CTX, char, node type, char, node expr) {
    auto res = create_node<types::Cast>(ctx, type->get_data().loc);
    ctx.ast.emplace_edge(res, type, types::Cast::TYPE);
    ctx.ast.emplace_edge(res, expr, types::Cast::VALUE);
    return res;
}

inline node make_if(CTX, str kw, char, node cond, char, node then_branch) {
    auto res = create_node<types::If>(ctx, sv2loc(kw));
    ctx.ast.emplace_edge(res, cond, types::If::COND);
    ctx.ast.emplace_edge(res, then_branch, types::If::IF_STMT);
    return res;
}

inline node make_if_else(
    CTX, str kw, char, node cond, char, node then_branch, str, node else_branch
) {
    auto res = create_node<types::If>(ctx, sv2loc(kw));
    ctx.ast.emplace_edge(res, cond, types::If::COND);
    ctx.ast.emplace_edge(res, then_branch, types::If::IF_STMT);
    ctx.ast.emplace_edge(res, else_branch, types::If::ELSE_STMT);
    return res;
}

inline node make_return(CTX, str kw, node value, char) {
    auto res = create_node<types::Return>(ctx, sv2loc(kw));
    ctx.ast.emplace_edge(res, value, 0);
    return res;
}

inline node make_call(CTX, node callee, char, list &&args, char) {
    auto res = create_node<types::Call>(ctx, callee->get_data().loc);
    ctx.ast.emplace_edge(res, callee, types::Call::EXPR);
    int idx = types::Call::ARG0;
    for (auto arg : args) { ctx.ast.emplace_edge(res, arg, idx++); }
    return res;
}

// clang-format off
inline node make_for(
    CTX, str kw, char,
    list init, char,
    std::optional<node> cond, char,
    list step, char,
    node body
) {
    auto res = create_node<types::For>(ctx, sv2loc(kw));
    if (!init.empty()) {
        auto init_node = create_node<types::ForInit>(
            ctx, init[0]->get_data().loc
        );
        for (auto n : init) {
            ctx.ast.emplace_edge(
                init_node, n, init_node->forward_edges().size()
            );
        }
        ctx.ast.emplace_edge(res, init_node, types::For::INIT);
    }
    if (cond.has_value()) {
        ctx.ast.emplace_edge(res, cond.value(), types::For::COND);
    }
    if (!step.empty()) {
        auto step_node = create_node<types::ForStep>(
            ctx, step[0]->get_data().loc
        );
        for (auto n : step) {
            ctx.ast.emplace_edge(
                step_node, n, step_node->forward_edges().size()
            );
        }
        ctx.ast.emplace_edge(res, step_node, types::For::STEP);
    }
    ctx.ast.emplace_edge(res, body, types::For::STMT);
    return res;
}
// clang-format on

inline node add_const(CTX, str sv, node type) {
    // TODO: if already add, insert warning
    type->get_data().get<ast::types::TypeSpec>().add_const();
    return type;
}

inline node add_volatile(CTX, str sv, node type) {
    // TODO: if already add, insert warning
    type->get_data().get<ast::types::TypeSpec>().add_volatile();
    return type;
}

inline node add_deref(CTX, node type, str sv) {
    auto child = create_node<types::Pointer>(ctx, sv2loc(sv));
    ctx.ast.emplace_edge(type, child, type->forward_edges().size());
    return type;
}

inline node add_subscript(CTX, node type, char, node expr, char) {
    ctx.ast.emplace_edge(type, expr, type->forward_edges().size());
    return type;
}

inline node add_global_stmt(CTX, node root, node stmt) {
    ctx.ast.emplace_edge(root, stmt, root->forward_edges().size());
    return root;
}

inline node connect_with_body(CTX, node fn_decl, node blk) {
    ctx.ast.emplace_edge(fn_decl, blk, types::Function::BODY);
    return fn_decl;
}

inline node make_binary_op(ParserContext &ctx, node l, str op, node r) {
    auto res = create_node<types::BinaryOp>(ctx, sv2loc(op), op);
    ctx.ast.emplace_edge(res, l, types::BinaryOp::LEFT);
    ctx.ast.emplace_edge(res, r, types::BinaryOp::RIGHT);
    return res;
}

inline node make_binary_assign(ParserContext &ctx, node l, str op, node r) {
    auto operation = make_binary_op(ctx, l, op.substr(0, op.size() - 1), r);
    auto res = ctx.ast.emplace_node(l->get_data());
    return make_assign(ctx, res, op, operation);
}

inline node make_unary_op(ParserContext &ctx, str op, node r) {
    auto res = create_node<types::UnaryOp>(ctx, sv2loc(op), op);
    ctx.ast.emplace_edge(res, r, 0);
    return res;
}

}; // namespace frontend::parser
