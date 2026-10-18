#include "frontend/ast/type_info.hpp"
#include "frontend/ast/types.hpp"
#include "frontend/parser/source_map.hpp"
#include <algorithm>
#include <cassert>
#include <frontend/ast/ast.hpp>
#include <frontend/parser/context.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace frontend::ast {

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
    struct name_data {
        std::string_view name;
        Ast::node_iter decl;
    };
    std::vector<name_data> names;
    const node_t *of_block;
    bool is_cycle;
};

static node_type lang_char_t;
static node_type lang_string_t;

struct context {
    using node = Ast::node_iter;
    using edge = Ast::edge_iter;
    using loc_t = parser::SourceLocation;

    parser::ParserContext &pctx;
    std::vector<ctx_layer> var_ctx;
    std::vector<ctx_layer::name_data> functions;

    int find_layer(const node_t &of_block) const;
    int push_layer(const node_t &of_block, bool is_cycle);
    void pop_layer();
    ctx_layer &top_layer();

    bool is_in_cycle(int level) const;

    bool is_name_defined(std::string_view name);
    bool insert_name(std::string_view name, node decl);

    ctx_layer::name_data &get_name(std::string_view name);

    ctx_layer::name_data *
    get_name_from_layer(std::string_view name, ctx_layer *layer = nullptr);

    type_info &get_type(const std::string &name);
    type_info &get_type(unsigned idx);
    std::optional<node_type> make_node_type(node n);

    std::optional<node_type> type_of_name(std::string_view id);
    node decl_of_name(std::string_view name);

    bool insert_function(std::string_view name, node decl);
    ctx_layer::name_data *get_function(std::string_view name);
    bool is_function_defined(std::string_view name);

    bool define_name(node n);
    bool define_type(node type);
    bool define_function(node n);

    bool check_global_names();

    bool check_function(node fn);
    bool check_block(node blk);
    bool check_stmt(node stmt);
    bool check_expr(node expr);

    std::optional<types::Literal> reduce_constant(node n);

    bool check_type_exists(node type);

    void insert_error(const std::string &msg, const loc_t &loc);
    void remove_node(node n);
};

void context::remove_node(node n) {
#ifdef AST_REMOVE_UNUSED_NODES
    pctx.ast.erase_subtree(n);
#endif
}

std::optional<node_type> context::type_of_name(std::string_view id) {
    node decl = decl_of_name(id);
    return decl->get_data().type;
}

context::node context::decl_of_name(std::string_view id) {
    assert(is_name_defined(id));
    return get_name(id).decl;
}

std::optional<types::Literal> context::reduce_constant(node n) {
    auto &n_data = n->get_data();

    if (n_data.is<types::Literal>()) return n_data.get<types::Literal>();

    if (n_data.is<types::BinaryOp>()) {
        auto left = reduce_constant(get_node(n, types::BinaryOp::LEFT));
        auto right = reduce_constant(get_node(n, types::BinaryOp::RIGHT));
        if (!left || !right) return std::nullopt;
        return left->apply(n_data.get<types::BinaryOp>().op, *right);
    }

    if (n_data.is<types::UnaryOp>()) {
        auto arg = reduce_constant(get_node(n, types::UnaryOp::ARG));
        if (!arg) return std::nullopt;
        return arg->apply(n_data.get<types::UnaryOp>().op);
    }

    insert_error(
        "Got non-constant expression in constant context", n->get_data().loc
    );
    return std::nullopt;
}

std::optional<node_type> context::make_node_type(node n) {
    assert(n->get_data().is<types::TypeSpec>());

    if (!check_type_exists(n)) { return std::nullopt; }

    auto &n_data = n->get_data();
    auto &ts = n_data.get<types::TypeSpec>();

    type_info &base_type = get_type(ts.name);
    node_type type{.type_id = base_type.type_id};

    for (auto e : n->forward_edges()) {
        auto &e_data = e->get_data();
        if (e_data.data < types::TypeSpec::DIM0) continue;

        node dim = e->target();
        if (dim->get_data().is<types::Deref>()) {
            type.insert_pointer();
        } else if (dim->get_data().is<types::Const>()) {
            type.insert_const();
        } else {
            auto idx_o = reduce_constant(dim);
            if (!idx_o) return std::nullopt;
            type.insert_array((size_t)*idx_o);
        }
    }

    return type;
}

bool context::define_type(node n) {
    assert(n->get_data().is<types::Structure>());

    auto &n_data = n->get_data();
    node t = get_node(n, types::Structure::TYPE);
    auto &t_data = t->get_data().get<types::TypeSpec>();

    unsigned type_id = 0;

    // at this moment the type might be already declared (but not fully defined)
    // so, get the existing type_info, or create the new one
    if (pctx.ast.types.has_type(t_data.name)) {
        type_id = pctx.ast.types.get(t_data.name).type_id;
    } else {
        type_id = pctx.ast.types.insert_type(
            {.name = t_data.name, .is_integral = false}
        );
    }

    type_info &new_tp = get_type(type_id);

    // if type has no body, no other attions required
    if (!n_data.get<types::Structure>().has_body) return true;

    // check if type is fully defined and if so, raise exception
    if (new_tp.size != 0) {
        insert_error("redefinition of type " + t_data.name, n_data.loc);
        return false;
    }

    // then define the fields of the type
    for (auto e : n->forward_edges()) {
        // search for start of field nodes
        if (e->get_data().data < types::Structure::FIELD0) continue;

        // current node is declareation of new field
        node decl = e->target();
        auto &decl_data = decl->get_data().get<types::Declare>();

        // get type of field. is must be already computed
        node field = get_node(decl, types::Declare::TYPE);
        auto field_type = make_node_type(field);
        if (!field_type) return false;

        if (field_type->size_of(pctx.ast.types) == 0) {
            insert_error(
                "type of field " + decl_data.name + " is not fully defined",
                decl->get_data().loc
            );
            return false;
        }

        // add field to type
        const std::string &name = decl_data.name;
        if (!new_tp.add_field(name, *field_type, pctx.ast.types)) {
            insert_error(
                "field with such name already defined", field->get_data().loc
            );
            return false;
        }
    }

    new_tp.update_size();

    return true;
}

type_info &context::get_type(const std::string &name) {
    return pctx.ast.types.get(name);
}

type_info &context::get_type(unsigned idx) { return pctx.ast.types.get(idx); }

bool context::check_type_exists(node type) {
    std::string &name = type->get_data().get<types::TypeSpec>().name;
    if (!pctx.ast.types.has_type(name)) {
        insert_error("type " + name + " is not defined", type->get_data().loc);
        return false;
    }
    return true;
}

bool context::define_name(node nd) {
    node_t &n = nd->get_data();
    assert(n.is<types::Declare>());
    auto &name = n.get<types::Declare>().name;
    node decl_type = get_node(nd, types::Declare::TYPE);

    if (!(n.type = make_node_type(decl_type))) return false;

    if (!insert_name(name, nd)) {
        insert_error(std::format("redefinition of name `{}`", name), n.loc);
        return false;
    }

    remove_node(decl_type);
    return true;
}

bool context::define_function(node n) {
    node_t &d = n->get_data();
    assert(d.is<types::Declare>());
    auto &name = d.get<types::Declare>().name;
    if (!insert_function(name, n)) {
        insert_error(std::format("redefinition of function `{}`", name), d.loc);
        return false;
    }
    return true;
}

bool context::insert_function(std::string_view name, node decl) {
    if (is_function_defined(name)) return false;
    functions.push_back({name, decl});
    return true;
}

ctx_layer::name_data *context::get_function(std::string_view name) {
    auto it =
        std::ranges::find_if(functions, [name](const ctx_layer::name_data &d) {
            return d.name == name;
        });
    if (it == functions.end()) return nullptr;
    return &*it;
}

bool context::is_function_defined(std::string_view name) {
    return get_function(name) != nullptr;
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

    var_ctx.emplace_back(
        std::vector<ctx_layer::name_data>{}, &of_block, is_cycle
    );
    return var_ctx.size() - 1;
}

void context::pop_layer() { var_ctx.pop_back(); }

ctx_layer &context::top_layer() { return var_ctx.back(); }

ctx_layer::name_data *
context::get_name_from_layer(std::string_view name, ctx_layer *layer) {
    if (layer == nullptr) layer = &top_layer();
    auto it = std::ranges::find_if(
        layer->names,
        [name](const ctx_layer::name_data &d) { return d.name == name; }
    );
    if (it == layer->names.end()) return nullptr;
    return &(*it);
}

ctx_layer::name_data &context::get_name(std::string_view name) {
    for (auto &l : var_ctx) {
        if (auto *n = get_name_from_layer(name, &l)) return *n;
    }

    assert(false && "name is not defined");
}

bool context::is_name_defined(std::string_view name) {
    for (auto &l : var_ctx) {
        if (get_name_from_layer(name, &l)) return true;
    }
    return false;
}

bool context::insert_name(std::string_view name, node decl) {
    if (get_name_from_layer(name)) return false;
    top_layer().names.emplace_back(name, decl);
    return true;
}

bool context::check_global_names() {
    Ast &ast = pctx.ast;
    node root = ast.get_root();

    std::vector<node> to_remove;

    assert(root->get_data().is<types::Root>());

    int level = push_layer(root->get_data(), /* is_cycle */ false);
    for (int i = 0; i < root->forward_edges().size(); ++i) {
        edge e = root->forward_edges()[i];
        node stmt = e->target();
        node_t &d = stmt->get_data();

        if (d.is<types::Declare>()) {
            define_name(stmt);
        } else if (d.is<types::Assign>()) {
            if (!define_name(get_node(stmt, types::Assign::LEFT))) continue;
        } else if (d.is<types::Structure>()) {
            define_type(stmt);
            to_remove.push_back(stmt);
        } else if (d.is<types::Function>()) {
            check_function(stmt);
        } else {
            assert("Invalid node type" == 0);
        }
    }

    for (auto n : to_remove) remove_node(n);

    return pctx.errors.empty();
}

bool context::check_function(node fn) {
    node decl = get_node(fn, types::Function::DECL);
    node_t &decl_data = decl->get_data();
    auto &decl_s = decl_data.get<types::Declare>();

    if (!define_function(decl)) return false;

    node decl_type = get_node(decl, types::Declare::TYPE);
    auto return_type = make_node_type(decl_type);
    if (!return_type) return false;

    push_layer(fn->get_data(), /* is_cycle */ false);

    type_info fn_type{
        .name = decl_s.name,
        .size = 1,
        .align = 1,
        .is_integral = false,
        .return_type = *return_type,
        .is_function = true,
    };

    bool ok = true;
    for (edge e : fn->forward_edges()) {
        if (e->get_data().data < types::Function::ARG0) continue;

        node arg = e->target();
        if (!define_name(arg)) {
            ok = false;
            continue;
        }

        auto &arg_s = arg->get_data().get<types::Declare>();
        fn_type.fields.push_back({
            .type = *arg->get_data().type,
            .name = arg_s.name,
        });
    }

    if (!ok) {
        pop_layer();
        return false;
    }

    decl_data.type =
        node_type{.type_id = pctx.ast.types.insert_type(std::move(fn_type))};

    auto body_it = find_edge(fn, types::Function::BODY);
    if (body_it != fn->forward_edges().end()) check_block((*body_it)->target());

    pop_layer();

    if (pctx.errors.empty()) remove_node(decl_type);
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

    if (d.is<types::Declare>()) { return define_name(stmt); }

    if (d.is<types::Assign>()) {
        check_expr(get_node(stmt, types::Assign::RIGHT));
        auto l = get_node(stmt, types::Assign::LEFT);
        if (l->get_data().is<types::Declare>()) return define_name(l);
        else return check_expr(l);
    }

    if (d.is<types::Return>()) { return check_expr(get_node(stmt, 0)); }

    if (d.is<types::If>()) {
        bool res;
        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data == types::If::COND)
                res &= check_expr(e->target());
            else res &= check_stmt(e->target());
        }
        return res;
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
        if (!is_name_defined(name) && !is_function_defined(name)) {
            insert_error(std::format("undefined name `{}`", name), d.loc);
            return false;
        }
        d.type = type_of_name(name);
        return true;
    }

    if (d.is<types::CharLiteral>()) {
        d.type = lang_char_t;
        return true;
    }

    if (d.is<types::StringLiteral>()) {
        d.type = lang_string_t;
        return true;
    }

    if (d.is<types::Literal>()) return true;

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
                if (ctx_layer::name_data *fn = get_function(fname)) {
                    auto &fn_type = fn->decl->get_data().type;
                    if (fn_type) d.type = fn_type->return_type(pctx.ast.types);
                } else {
                    insert_error(
                        std::format("unknown function `{}`", fname), cd.loc
                    );
                }
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

    lang_char_t = node_type{.type_id = types.get_id("int8_t")};
    lang_string_t = node_type{.type_id = types.get_id("int8_t")};
    lang_string_t.insert_const();
    lang_string_t.insert_pointer();

    c.check_global_names();
    return ctx.errors.empty();
}

}; // namespace frontend::ast