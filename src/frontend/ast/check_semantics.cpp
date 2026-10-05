#include "frontend/ast/types.hpp"
#include "frontend/parser/source_map.hpp"
#include <algorithm>
#include <cassert>
#include <frontend/ast/ast.hpp>
#include <frontend/parser/context.hpp>
#include <vector>

namespace frontend::ast {

/**
 * Для проверки корректности имён следует обойти граф в ширину и по возрастанию
 * номера потомка.
 *
 * Интерес представляют узлы:
 * - ID -- проверить, что уже определено
 * - Declare -- проверить, что ещё не определено и определить.
 *
 * Важен контекст определения и имени. Контекст задаёт родительсикй узел.
 * Конктерно, decalre может относиться:
 * - к переменной:
 *   - function::ARGn
 *   - block::STMTn
 *   - assign::LEFT
 * - к функции:
 *   - function::DECL
 *
 * ID может относиться:
 * - к имени функции:
 *   - call::EXPR
 * - к имени переменной - в остальных случаях
 *
 * Дополнительные проверки:
 * - выражениение вызова функции дложно быть всегда единичным узлом ID
 *   возможность указать expression была взята как задаток для указатеелй на
 *   функции
 * - break и continue должны встречаться только в теле цикла. пока что они
 *   могут встречаться ещё и в контексте функции. Так сделано чтобы не
 *   дублировать выражения в грамматике
 */

inline auto find_edge(Ast::node_iter n, int data) {
    return std::ranges::find_if(n->forward_edges(), [data](auto e) {
        return e->get_data().data == data;
    });
}

inline Ast::edge_iter get_edge(Ast::node_iter n, int data) {
    auto it = find_edge(n, data);
    assert(it != n->forward_edges().end());
    return *it;
}

inline Ast::node_iter get_node(Ast::node_iter n, int data) {
    return get_edge(n, data)->target();
}

template <typename T> inline T &get_node_data(Ast::node_iter n, int data) {
    Ast::node_iter it = get_node(n, data);
    auto &val = it->get_data();
    assert(val.is<T>());
    return val.get<T>();
}

struct ctx_layer {
    std::vector<std::string_view> names;
    const node_t *of_block;
    bool is_cycle;
};

struct context {
    using node = Ast::node_iter;
    using edge = Ast::edge_iter;
    using loc_t = parser::SourceLocation;

    parser::ParserContext &pctx;
    std::vector<ctx_layer> var_ctx;
    std::vector<std::string> functions;

    int find_layer(const node_t &of_block) const;
    int push_layer(const node_t &of_block, bool is_cycle);
    void pop_layer();
    ctx_layer &top_layer();

    bool is_in_cycle(int level) const;

    bool is_name_defined(std::string_view name);
    bool has_name(std::string_view name);
    bool insert_name(std::string_view name);

    bool insert_function(std::string_view name);
    bool is_function_defined(std::string_view name);

    void insert_error(const std::string &msg, const loc_t &loc);
    void define_name(node_t n);
    void define_function(node_t n);

    bool check_global_names();

    bool check_function(node fn);
    bool check_block(node blk);
    bool check_stmt(node stmt);
    bool check_expr(node expr);
};

void context::define_name(node_t n) {
    assert(n.is<types::Declare>());
    auto &name = n.get<types::Declare>().name;
    if (!insert_name(name)) {
        insert_error(std::format("redefinition of name `{}`", name), n.loc);
    }
}

void context::define_function(node_t n) {
    assert(n.is<types::Declare>());
    auto &name = n.get<types::Declare>().name;
    if (!insert_function(name)) {
        insert_error(std::format("redefinition of function `{}`", name), n.loc);
    }
}

bool context::insert_function(std::string_view name) {
    if (is_function_defined(name)) return false;
    functions.emplace_back(name);
    return true;
}

bool context::is_function_defined(std::string_view name) {
    return std::ranges::find(functions, name) != functions.end();
}

void context::insert_error(const std::string &msg, const loc_t &loc) {
    pctx.errors.emplace_back(msg, loc);
}

bool context::is_in_cycle(int level) const {
    for (auto &l : var_ctx) {
        if (l.is_cycle) return true;
    }
    return false;
}

int context::find_layer(const node_t &of_block) const {
    for (int i = var_ctx.size() - 1; i >= 0; --i) {
        if (var_ctx[i].of_block == &of_block) return i;
    }
    return -1;
}

int context::push_layer(const node_t &of_block, bool is_cycle) {
    assert(find_layer(of_block) == -1);
    assert(!is_cycle || is_cycle && var_ctx.size() >= 2);

    var_ctx.emplace_back(std::vector<std::string_view>{}, &of_block, is_cycle);
    return var_ctx.size() - 1;
}

void context::pop_layer() { var_ctx.pop_back(); }

ctx_layer &context::top_layer() { return var_ctx.back(); }

bool context::has_name(std::string_view name) {
    return std::ranges::find(top_layer().names, name)
        != top_layer().names.end();
}

bool context::is_name_defined(std::string_view name) {
    for (auto &l : var_ctx) {
        if (std::ranges::find(l.names, name) != l.names.end()) return true;
    }
    return false;
}

bool context::insert_name(std::string_view name) {
    if (has_name(name)) return false;
    top_layer().names.push_back(name);
    return true;
}

bool context::check_global_names() {
    Ast &ast = pctx.ast;
    node root = ast.get_root();

    assert(root->get_data().is<types::Root>());

    int level = push_layer(root->get_data(), /* is_cycle */ false);
    for (edge e : root->forward_edges()) {
        node stmt = e->target();
        node_t &d = stmt->get_data();

        if (d.is<types::Declare>()) {
            define_name(d);
        } else if (d.is<types::Assign>()) {
            define_name(get_node(stmt, types::Assign::LEFT)->get_data());
        } else if (d.is<types::Function>()) {
            check_function(stmt);
        } else {
            assert("Invalid node type" == 0);
        }
    }

    return pctx.errors.empty();
}

bool context::check_function(node fn) {
    define_function(get_node(fn, types::Function::DECL)->get_data());

    push_layer(fn->get_data(), /* is_cycle */ false);
    for (edge e : fn->forward_edges()) {
        if (e->get_data().data >= types::Function::ARG0)
            define_name(e->target()->get_data());
    }

    auto body_it = find_edge(fn, types::Function::BODY);
    if (body_it != fn->forward_edges().end()) check_block((*body_it)->target());

    pop_layer();
    return pctx.errors.empty();
}

bool context::check_block(node blk) {
    push_layer(blk->get_data(), /* is_cycle */ false);
    for (edge e : blk->forward_edges()) check_stmt(e->target());
    pop_layer();
    return pctx.errors.empty();
}

bool context::check_stmt(node stmt) {
    node_t &d = stmt->get_data();

    if (d.is<types::Block>()) { return check_block(stmt); }

    if (d.is<types::Declare>()) {
        define_name(d);
        return true;
    }

    if (d.is<types::Assign>()) {
        check_expr(get_node(stmt, types::Assign::RIGHT));
        auto l = get_node(stmt, types::Assign::LEFT);
        if (l->get_data().is<types::Declare>()) define_name(l->get_data());
        else check_expr(l);
        return true;
    }

    if (d.is<types::Return>()) {
        check_expr(get_node(stmt, 0));
        return true;
    }

    if (d.is<types::If>()) {
        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data == types::If::COND) check_expr(e->target());
            else check_stmt(e->target());
        }
        return true;
    }

    if (d.is<types::For>()) {
        push_layer(d, /* is_cycle */ true);
        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data == types::For::COND) check_expr(e->target());
            else check_stmt(e->target());
        }
        pop_layer();
        return true;
    }

    if (d.is<types::ForInit>() || d.is<types::ForStep>()) {
        for (edge e : stmt->forward_edges()) check_stmt(e->target());
        return true;
    }

    check_expr(stmt);
    return true;
}

bool context::check_expr(node expr) {
    node_t &d = expr->get_data();

    if (d.is<types::ID>()) {
        const std::string &name = d.get<types::ID>().name;
        if (!is_name_defined(name) && !is_function_defined(name))
            insert_error(std::format("undefined name `{}`", name), d.loc);
        return true;
    }

    if (d.is<types::Literal>() || d.is<types::CharLiteral>()
        || d.is<types::StringLiteral>())
        return true;

    if (d.is<types::Call>()) {
        for (edge e : expr->forward_edges()) {
            node child = e->target();
            if (e->get_data().data == types::Call::EXPR) {
                node_t &cd = child->get_data();
                if (!cd.is<types::ID>()) {
                    insert_error("call target must be an identifier", cd.loc);
                    continue;
                }
                const std::string &fname = cd.get<types::ID>().name;
                if (!is_function_defined(fname))
                    insert_error(
                        std::format("unknown function `{}`", fname), cd.loc
                    );
            } else {
                check_expr(child);
            }
        }
        return true;
    }

    if (d.is<types::Cast>()) {
        for (edge e : expr->forward_edges()) {
            if (e->get_data().data == types::Cast::TYPE) continue;
            check_expr(e->target());
        }
        return true;
    }

    for (edge e : expr->forward_edges()) check_expr(e->target());
    return true;
}

bool Ast::check_semantics(parser::ParserContext &ctx) {
    context c{.pctx = ctx};
    c.check_global_names();
    return ctx.errors.empty();
}

}; // namespace frontend::ast
