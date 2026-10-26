#include "frontend/ast/ast.hpp"
#include "frontend/ast/type_info.hpp"
#include "frontend/ast/types.hpp"
#include "frontend/parser/context.hpp"
#include "frontend/parser/source_map.hpp"
#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
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

/** @brief get integer type description if @p type is not a pointer/array */
static const type_info *as_integer(const node_type &type, TypeTable &tb) {
    if (!type.mods.empty()) return nullptr;

    const type_info &info = tb.get(type.type_id);
    if (info.is_function || !info.is_integral) return nullptr;
    return &info;
}

/** @brief build a basic integer type of the given size and signedness */
static node_type
make_integer_type(TypeTable &tb, uint8_t size, bool is_signed) {
    std::string_view name;
    switch (size) {
    case 1: name = is_signed ? "int8_t" : "uint8_t"; break;
    case 2: name = is_signed ? "int16_t" : "uint16_t"; break;
    case 4: name = is_signed ? "int32_t" : "uint32_t"; break;
    case 8: name = is_signed ? "int64_t" : "uint64_t"; break;
    default:
        if (size <= 1) name = is_signed ? "int8_t" : "uint8_t";
        else if (size <= 2) name = is_signed ? "int16_t" : "uint16_t";
        else if (size <= 4) name = is_signed ? "int32_t" : "uint32_t";
        else name = is_signed ? "int64_t" : "uint64_t";
        break;
    }
    return node_type{.type_id = tb.get_id(name)};
}

/**
 * @brief usual arithmetic conversion of two integer operands
 *
 * Both operands undergo the integer promotions first. The result has the size
 * of the wider operand and is signed only if both operands are signed (or the
 * wider one is signed), matching the C usual arithmetic conversions.
 */
static node_type
common_integer_type(TypeTable &tb, const node_type &lhs, const node_type &rhs) {
    const type_info &l = tb.get(lhs.type_id);
    const type_info &r = tb.get(rhs.type_id);

    const uint8_t l_size = l.size < 4 ? 4 : l.size;
    const bool l_signed = l.size < 4 ? true : l.is_signed;
    const uint8_t r_size = r.size < 4 ? 4 : r.size;
    const bool r_signed = r.size < 4 ? true : r.is_signed;

    uint8_t size;
    bool is_signed;

    if (l_size == r_size) {
        size = l_size;
        is_signed = l_signed && r_signed;
    } else if (l_size > r_size) {
        size = l_size;
        is_signed = l_signed;
    } else {
        size = r_size;
        is_signed = r_signed;
    }

    return make_integer_type(tb, size, is_signed);
}

/**
 * @brief type produced by the boolean/relational operators
 *
 * C-like languages produce a plain `int` for such operators.
 */
static node_type boolean_type(TypeTable &tb) {
    return make_integer_type(tb, 4, true);
}

/**
 * @brief get the type of the value a pointer or an array gives access to
 *
 * Modifiers are applied from right to left, so the last one is the outermost
 * and it is the one a subscription or a dereference removes. The `const` of
 * that level belongs to the removed object and is dropped together with it
 * (`uint32_t const *` -> `uint32_t`).
 */
static std::optional<node_type> pointee_type(const node_type &type) {
    if (type.mods.empty()) return std::nullopt;

    node_type result = type;
    result.mods.pop_back();
    return result;
}

/**
 * @brief why a value of @p type cannot be assigned to, or null if it can
 *
 * `const` qualifies the object it is written at, so only the outermost level
 * of the type matters: `uint32_t const *` is a const pointer which cannot be
 * reassigned, while `const uint32_t *` points to a value which cannot be
 * assigned through the pointer. An array is not assignable at all, because
 * its name denotes all of the elements at once.
 */
static const char *assign_error(const node_type &type) noexcept {
    if (!type.mods.empty()
        && type.mods.back().kind == node_type::modifier::Kind::Array)
        return "cannot assign to an array";

    const bool is_const =
        type.mods.empty() ? type.is_const : type.mods.back().is_const;
    if (is_const) return "cannot assign to a const value";

    return nullptr;
}

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

    bool check_assignment(node expr);
    bool check_binary_op(node expr);
    bool check_unary_op(node expr);
    bool check_subscript(node expr);
    bool check_deref(node expr);
    bool check_addr(node expr);
    bool check_get(node expr);
    bool check_cast(node expr);
    bool check_call(node expr);

    std::optional<types::Literal> reduce_constant(node n);

    bool check_type_exists(node type);

    bool insert_error(const std::string &msg, const loc_t &loc);
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
        if (dim->get_data().is<types::Ptr>()) {
            type.insert_pointer(dim->get_data().get<types::Ptr>().is_const);
        } else if (dim->get_data().is<types::Arr>()) {
            auto idx_o = reduce_constant(dim);
            if (!idx_o) return std::nullopt;
            type.insert_array(
                (size_t)*idx_o, dim->get_data().get<types::Arr>().is_const
            );
        } else {
            assert(false && "invlaid node type");
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
        return insert_error("redefinition of type " + t_data.name, n_data.loc);
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

        decl->get_data().type = *field_type;

        if (field_type->size_of(pctx.ast.types) == 0) {
            return insert_error(
                "type of field " + decl_data.name + " is not fully defined",
                decl->get_data().loc
            );
        }

        // add field to type
        const std::string &name = decl_data.name;
        if (!new_tp.add_field(name, *field_type, pctx.ast.types)) {
            return insert_error(
                "field with such name already defined", field->get_data().loc
            );
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
        return insert_error(
            "type " + name + " is not defined", type->get_data().loc
        );
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
        return insert_error(
            std::format("redefinition of name `{}`", name), n.loc
        );
    }

    remove_node(decl_type);
    return true;
}

bool context::define_function(node n) {
    node_t &d = n->get_data();
    assert(d.is<types::Declare>());
    auto &name = d.get<types::Declare>().name;
    if (!insert_function(name, n)) {
        return insert_error(
            std::format("redefinition of function `{}`", name), d.loc
        );
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

bool context::insert_error(const std::string &msg, const loc_t &loc) {
    pctx.errors.emplace_back(msg, loc);
    return false;
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
    for (auto &l : std::ranges::views::reverse(var_ctx)) {
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
            check_assignment(stmt);
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
    if (d.is<types::Assign>()) { return check_assignment(stmt); }

    if (d.is<types::Return>()) {
        // TODO: check if return statement type is equal to function return type
        return check_expr(get_node(stmt, 0));
    }

    if (d.is<types::If>()) {
        bool ok;
        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data == types::If::COND) {
                ok &= check_expr(e->target());
                // TODO: need to check if e->target() of integral, non-array and
                // non-pointer type
            } else ok &= check_stmt(e->target());
        }
        return ok;
    }

    if (d.is<types::For>()) {
        push_layer(d, /* is_cycle */ true);
        bool ok;
        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data == types::For::COND) {
                ok &= check_expr(e->target());
                // TODO: need to check if e->target() of integral, non-array and
                // non-pointer type
            } else ok &= check_stmt(e->target());
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
    TypeTable &tb = pctx.ast.types;

    if (d.is<types::ID>()) {
        const std::string &name = d.get<types::ID>().name;
        if (is_name_defined(name)) {
            d.type = type_of_name(name);
            return true;
        }
        if (ctx_layer::name_data *fn = get_function(name)) {
            d.type = fn->decl->get_data().type;
            return true;
        }
        return insert_error(std::format("undefined name `{}`", name), d.loc);
    }

    if (d.is<types::CharLiteral>()) {
        d.type = lang_char_t;
        return true;
    }

    if (d.is<types::StringLiteral>()) {
        d.type = lang_string_t;
        return true;
    }

    if (d.is<types::Literal>()) {
        auto &lit = d.get<types::Literal>();
        d.type = make_integer_type(tb, lit.size, lit.is_signed);
        return true;
    }

    if (d.is<types::Assign>()) return check_assignment(expr);
    if (d.is<types::Declare>()) return define_name(expr);
    if (d.is<types::BinaryOp>()) return check_binary_op(expr);
    if (d.is<types::UnaryOp>()) return check_unary_op(expr);
    if (d.is<types::Subscript>()) return check_subscript(expr);
    if (d.is<types::Deref>()) return check_deref(expr);
    if (d.is<types::Addr>()) return check_addr(expr);
    if (d.is<types::Get>()) return check_get(expr);
    if (d.is<types::Cast>()) return check_cast(expr);
    if (d.is<types::Call>()) return check_call(expr);

    for (edge e : expr->forward_edges()) check_expr(e->target());
    return true;
}

bool context::check_assignment(node expr) {
    node right = get_node(expr, types::Assign::RIGHT);
    if (!check_expr(right)) return false;

    node left = get_node(expr, types::Assign::LEFT);
    if (!check_expr(left)) return false;

    // TODO: need to implement check_assignment function:
    // * types of left and right must be strictly equal if right is not a
    //   literal
    // * if right is literal, it must be implicuty converted to the type
    //   of left
    // * if left is const type or an array - it may not be assigned

    return true;
}

bool context::check_binary_op(node expr) {
    node_t &d = expr->get_data();
    const std::string &op = d.get<types::BinaryOp>().op;
    TypeTable &tb = pctx.ast.types;

    node left = get_node(expr, types::BinaryOp::LEFT);
    node right = get_node(expr, types::BinaryOp::RIGHT);

    if (!check_expr(left)) return false;
    if (!check_expr(right)) return false;

    // till this moment types of left and right must be determined already
    // if not, something is wrong with other functions
    const auto &left_type = left->get_data().type;
    const auto &right_type = right->get_data().type;
    assert(left_type);
    assert(right_type);

    // binary operations is for integer-type operands only
    const type_info *left_info = as_integer(*left_type, tb);
    const type_info *right_info = as_integer(*right_type, tb);
    if (!left_info || !right_info) {
        return insert_error(
            std::format(
                "invalid operands `{}` and `{}` to binary operator `{}`",
                left_type->repr(tb),
                right_type->repr(tb),
                op
            ),
            d.loc
        );
    }

    // list of boolean operations
    if (op == "||" || op == "&&" || op == "==" || op == "!=" || op == "<"
        || op == ">" || op == "<=" || op == ">=") {
        // logic operation is of boolean type
        d.type = boolean_type(tb);
        // if size_of(left_type) != size_of(right_type) promote the shorter one
        // to the wider one
        return true;
    } else if (op == "<<" || op == ">>") {
        // shifts is of the same type as left operand
        d.type = left_type;
        return true;
    } else {
        // arithmetic operations result is of widest type
        d.type = common_integer_type(tb, *left_type, *right_type);
        // if size_of(left_type) != size_of(right_type) promote the shorter one
        // to the wider one
    }

    return true;
}

bool context::check_unary_op(node expr) {
    node_t &d = expr->get_data();
    const std::string &op = d.get<types::UnaryOp>().op;
    TypeTable &tb = pctx.ast.types;

    node arg = get_node(expr, types::UnaryOp::ARG);
    if (!check_expr(arg)) return false;

    const auto &arg_type = arg->get_data().type;
    assert(arg_type);

    // the only check required is that the type is integer
    const type_info *info = as_integer(*arg_type, tb);
    if (!info) {
        return insert_error(
            std::format(
                "invalid operand of type `{}` to unary operator `{}`",
                arg_type->repr(tb),
                op
            ),
            d.loc
        );
        d.type = make_integer_type(tb, 4, true);
    }

    if (op == "!") {
        d.type = boolean_type(tb);
        return true;
    }

    d.type = arg_type;
    return true;
}

bool context::check_subscript(node expr) {
    node_t &d = expr->get_data();

    node value = get_node(expr, types::Subscript::VALUE);
    node index = get_node(expr, types::Subscript::IDX);

    if (!check_expr(value)) return false;
    if (!check_expr(index)) return false;

    const auto &value_type = value->get_data().type;
    assert(value_type);

    auto element_type = pointee_type(*value_type);
    if (!element_type) {
        return insert_error(
            "subscripted value is not an array or a pointer", d.loc
        );
    }

    const auto &index_type = index->get_data().type;
    if (index_type && !as_integer(*index_type, pctx.ast.types)) {
        return insert_error("subscript index is not an integer", d.loc);
    }

    d.type = *element_type;
    return true;
}

bool context::check_deref(node expr) {
    node_t &d = expr->get_data();

    node value = get_node(expr, 0);
    if (!check_expr(value)) return false;

    const auto &value_type = value->get_data().type;
    assert(value_type);

    auto element_type = pointee_type(*value_type);
    if (!element_type) {
        return insert_error("cannot dereference a non-pointer value", d.loc);
    }

    d.type = *element_type;
    return false;
}

bool context::check_addr(node expr) {
    node_t &d = expr->get_data();

    // TODO: check if we are able to get address of value.
    // address may be taken from any in-memory value:
    // * ID (including function), Deref -- is always valid
    // * Get, Subscript -- is only valid when the object is not temporary (not
    //   returned by function)
    // * Cast -- valid if cast of one of mentioned
    // pointer arithmetic is not allowed, so &(a + b) is never valid

    node value = get_node(expr, 0);
    if (!check_expr(value)) return false;

    const auto &value_type = value->get_data().type;
    assert(value_type);

    d.type = *value_type;
    // make the type a constant pointer to the type the address is taken of
    // type_t const *ptr; - means ptr is not assignable, but *ptr does
    d.type->insert_pointer(true);
    return true;
}

bool context::check_get(node expr) {
    node_t &d = expr->get_data();
    const std::string &member = d.get<types::Get>().name;
    TypeTable &tb = pctx.ast.types;

    node base = get_node(expr, types::Get::BASE);

    // no matter which type is base, if check is true, the type is determined.
    if (!check_expr(base)) return false;

    const auto &base_type = base->get_data().type;
    if (!base_type) return false;

    if (!base_type->mods.empty() || base_type->is_function(tb)) {
        return insert_error(
            std::format(
                "member access on a non-struct value of type `{}`",
                base_type->repr(tb)
            ),
            d.loc
        );
    }

    const type_info &base_info = get_type(base_type->type_id);
    auto field = std::ranges::find_if(
        base_info.fields,
        [&member](const type_info::field &f) { return f.name == member; }
    );
    if (field == base_info.fields.end()) {
        return insert_error(
            std::format(
                "type `{}` has no member named `{}`",
                base_type->repr(tb),
                member
            ),
            d.loc
        );
    }

    d.type = field->type;
    return true;
}

bool context::check_cast(node expr) {
    node_t &d = expr->get_data();

    node value = get_node(expr, types::Cast::VALUE);
    if (!check_expr(value)) return false;

    // TODO: check if expression realy may be casted into this type

    node type_node = get_node(expr, types::Cast::TYPE);
    auto cast_type = make_node_type(type_node);
    if (cast_type) d.type = *cast_type;
    else return false;

    return true;
}

bool context::check_call(node expr) {
    node_t &d = expr->get_data();
    TypeTable &tb = pctx.ast.types;

    node callee = get_node(expr, types::Call::EXPR);

    // check if call arguments (including callee type) are valid expressions
    for (edge e : expr->forward_edges()) {
        if (!check_expr(e->target())) return false;
    }

    // now all the types are determined
    const auto &fn_type = callee->get_data().type;
    assert(fn_type);

    if (!fn_type->is_function(tb))
        return insert_error(
            std::format(
                "attempt to call non-function type `{}`", fn_type->repr(tb)
            ),
            d.loc
        );

    // TODO: check if type of expression is the same as function expects to be
    // passed, implicit cast is not allowed it this context

    d.type = fn_type->return_type(tb);
    return true;
}

bool Ast::check_semantics(parser::ParserContext &ctx) {
    context c{.pctx = ctx};

    lang_char_t = node_type{.type_id = types.get_id("int8_t")};
    lang_string_t =
        node_type{.type_id = types.get_id("int8_t"), .is_const = true};
    lang_string_t.insert_pointer(true);

    c.check_global_names();
    return ctx.errors.empty();
}

}; // namespace frontend::ast