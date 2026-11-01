#include "frontend/ast/ast.hpp"
#include "frontend/ast/type_info.hpp"
#include "frontend/ast/types.hpp"
#include "frontend/parser/context.hpp"
#include "frontend/parser/source_map.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#define PTR_SIZE 8

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

static node_type
make_integer_type(TypeTable &tb, uint8_t size, bool is_signed) {
    std::string_view name;
    switch (size) {
    case 1: name = is_signed ? "int8_t" : "uint8_t"; break;
    case 2: name = is_signed ? "int16_t" : "uint16_t"; break;
    case 4: name = is_signed ? "int32_t" : "uint32_t"; break;
    default: name = is_signed ? "int64_t" : "uint64_t"; break;
    }
    return node_type{.type_id = tb.get_id(name)};
}

/**
 * @brief result type of the arithmetic operators
 *
 * There is no integer promotion in this language: the result has the wider
 * operand size and, for equal sizes, it is unsigned if either operand is
 * unsigned.
 */
static node_type make_common_integer_type(
    TypeTable &tb, const node_type &lhs, const node_type &rhs
) {
    const type_info &l = tb.get(lhs.type_id);
    const type_info &r = tb.get(rhs.type_id);

    uint8_t size;
    bool is_signed;

    if (l.size == r.size) {
        size = l.size;
        is_signed = l.is_signed && r.is_signed;
    } else if (l.size > r.size) {
        size = l.size;
        is_signed = l.is_signed;
    } else {
        size = r.size;
        is_signed = r.is_signed;
    }
    return make_integer_type(tb, size, is_signed);
}

static node_type make_boolean_type(TypeTable &tb) {
    return make_integer_type(tb, 4, true);
}

struct scope_entry {
    std::string name;
    node_type type;
    bool is_global = false;
};

struct scope_layer {
    std::vector<scope_entry> names;
    bool is_loop = false;
};

struct function_entry {
    std::string name;
    node_type type{};
    std::optional<Ast::node_iter> body{};
    parser::SourceLocation loc;
};

struct validate_poolicy {
    bool accept_array   : 1 = true;
    bool accept_void    : 1 = true;
    bool accept_function: 1 = false;
};

enum const_policy {
    // policy is meaned to represent comparison of right type to left type
    STRICT, // left == right <=> left and right are both const or non-const
    IGNORE, // left == right despite any constantness
    MAY_BE_CONST,   // if left is const, right may be const if not
    MAY_BE_MUTIBLE, // if left is non-const, right may be non-const if not
};

struct context {
    using node = Ast::node_iter;
    using edge = Ast::edge_iter;
    using loc_t = parser::SourceLocation;

    parser::ParserContext &pctx;

    std::vector<scope_layer> scopes;
    std::vector<function_entry> functions;
    function_entry *current_function;

    TypeTable &types() { return pctx.ast.types; }

    void push_scope(bool is_loop);
    void pop_scope();
    scope_layer &top_scope();
    bool is_global_scope() const;
    bool is_in_loop() const;

    scope_entry *lookup_var(std::string_view name);
    scope_entry *lookup_global_var(std::string_view name);
    bool insert_name(
        std::string_view name,
        const node_type &type,
        bool is_global,
        const loc_t &loc
    );

    function_entry *get_function(std::string_view name);
    function_entry *register_function(
        std::string_view name, const node_type &type, const loc_t &loc
    );
    bool declare_variable(node decl);

    bool is_lvalue(node n, bool require_global = false);
    bool validate_type(
        const node_type &type,
        const loc_t &loc,
        std::string_view what,
        validate_poolicy p = {}
    );

    bool
    is_types_compatible(const node_type &a, const node_type &b, const_policy p);

    node_type make_function_type(
        const node_type &return_type, const std::vector<node_type> &params
    );
    std::optional<node_type> make_node_type(node n);
    std::optional<types::Literal> eval_const(node n);

    bool check_global_names();
    bool check_structure(node n);
    bool check_function(node n);
    bool check_main();

    bool check_block(node blk, bool make_scope);
    bool check_stmt(node stmt);
    bool check_if_stmt(node stmt);
    bool check_for_stmt(node stmt);

    /** check if subtree represents a valid expression
     *  secont argument is a type hint, which restricts expression to be this
     *  type and in some cases is used to determine the type of expression
     */
    bool check_expr(
        node expr, const node_type *as_type = nullptr, const_policy p = IGNORE
    );
    bool check_binary_op(node expr);
    bool check_unary_op(node expr);
    bool check_subscript(node expr);
    bool check_deref(node expr);
    bool check_addr(node expr);
    bool check_get(node expr);
    bool check_cast(node expr);
    bool check_call(node expr);
    bool check_sizeof(node expr);
    bool check_condition(node n);
    bool check_return(node n);
    bool check_initializer(node init, const node_type &type);
    bool check_assignment(node expr);

    bool insert_error(const std::string &msg, const loc_t &loc);
};

bool context::is_global_scope() const { return scopes.size() == 1; }

bool context::is_in_loop() const {
    for (auto &s : std::ranges::views::reverse(scopes)) {
        if (s.is_loop) return true;
    }
    return false;
}

void context::push_scope(bool is_loop) {
    scopes.push_back(scope_layer{{}, is_loop});
}

void context::pop_scope() {
    assert(!scopes.empty());
    scopes.pop_back();
}

scope_layer &context::top_scope() { return scopes.back(); }

scope_entry *context::lookup_var(std::string_view name) {
    for (auto &layer : std::views::reverse(scopes)) {
        auto it = std::ranges::find_if(layer.names, [name](auto &e) {
            return e.name == name;
        });
        if (it != layer.names.end()) return &*it;
    }
    return nullptr;
}

scope_entry *context::lookup_global_var(std::string_view name) {
    if (scopes.empty()) return nullptr;
    auto &layer = scopes.front();
    auto it = std::ranges::find_if(layer.names, [name](auto &e) {
        return e.name == name;
    });
    if (it == layer.names.end()) return nullptr;
    return &*it;
}

bool context::insert_name(
    std::string_view name,
    const node_type &type,
    bool is_global,
    const loc_t &loc
) {
    auto &layer = top_scope();
    auto it = std::ranges::find_if(layer.names, [name](auto &e) {
        return e.name == name;
    });
    if (it != layer.names.end()) {
        return insert_error(
            std::format("redefinition of name `{}`", name), loc
        );
    }

    if (is_global && get_function(name)) {
        return insert_error(
            std::format("name `{}` is already used by a function", name), loc
        );
    }

    layer.names.push_back({std::string(name), type, is_global});
    return true;
}

function_entry *context::get_function(std::string_view name) {
    auto it = std::ranges::find_if(functions, [name](auto &f) {
        return f.name == name;
    });
    if (it == functions.end()) return nullptr;
    return &*it;
}

function_entry *context::register_function(
    std::string_view name, const node_type &type, const loc_t &loc
) {
    if (get_function(name) || lookup_global_var(name)) {
        insert_error(
            std::format("redefinition of global name `{}`", name), loc
        );
        return nullptr;
    }
    functions.push_back({
        .name = std::string(name),
        .type = type,
        .loc = loc,
    });
    return &functions.back();
}

bool context::insert_error(const std::string &msg, const loc_t &loc) {
    pctx.errors.emplace_back(msg, loc);
    return false;
}

/** check if type b is compatible to type a */
bool context::is_types_compatible(
    const node_type &a, const node_type &b, const_policy p
) {
    static const TypeTable &tb = types();

    // soft variant -- all integer types are compatible to each other
    // if (a.is_integer(tb) && b.is_integer(tb)) return true;

    // or may be more strictly -- not to check const\non-const for integers
    if (a.is_integer(tb) && b.is_integer(tb)) p = IGNORE;

    auto check_const = [p](bool a, bool b) {
        switch (p) {
        case STRICT: return a == b;
        case IGNORE: return true;
        case MAY_BE_CONST: return a == b || a;
        case MAY_BE_MUTIBLE: return a == b || !a;
        default: assert("invalid const policy" && false);
        }
    };

    if (a.mods.size() != b.mods.size()) return false;
    if (!check_const(a._is_const, b._is_const)) return false;
    for (size_t i = 0; i < a.mods.size(); ++i) {
        if (a.mods[i].kind != b.mods[i].kind) return false;
        if (a.mods[i].count != b.mods[i].count) return false;
        if (!check_const(a.mods[i].is_const, b.mods[i].is_const)) return false;
    }

    if (a.type_id == b.type_id) return true;
    if (a.is_pointer(tb) && b.is_pointer(tb)) return true;

    const type_info &ia = tb.get(a.type_id);
    const type_info &ib = tb.get(b.type_id);

    if (!a.is_function(tb) || !b.is_function(tb)) return false;

    if (!is_types_compatible(ia.return_type, ib.return_type, p)) return false;
    if (ia.fields.size() != ib.fields.size()) return false;
    for (size_t i = 0; i < ia.fields.size(); ++i) {
        if (!is_types_compatible(ia.fields[i].type, ib.fields[i].type, p))
            return false;
    }
    return true;
}

bool context::validate_type(
    const node_type &type,
    const loc_t &loc,
    std::string_view what,
    validate_poolicy p
) {
    node_type t = type;
    const bool top_array = t.is_array(types());

    auto error = [this, what, &loc](std::string_view tp) {
        return insert_error(
            std::format("{} cannot be used as {}", tp, what), loc
        );
    };

    if (!p.accept_void && t.is_void(types())) return error("type `void_t`");
    if (!p.accept_array && t.is_array(types())) return error("array-type");
    if (!p.accept_function && t.is_function(types()))
        return error("a function type");

    while (t.is_array(types())) t.mods.pop_back();

    if (top_array && t.is_function(types()))
        return insert_error("array of functions is not a valid type", loc);

    if (top_array && t.is_void(types()))
        return insert_error("array of void_t is not a valid type", loc);

    if (t.size_of(types()) == 0)
        return error(std::format("incomplete type `{}`", type.repr(types())));

    return true;
}

node_type context::make_function_type(
    const node_type &return_type, const std::vector<node_type> &params
) {
    type_info fn;
    fn.name = "";
    fn.size = 0;
    fn.align = 1;
    fn.is_function = true;
    fn.return_type = return_type;
    for (const auto &p : params) {
        fn.fields.push_back({.type = p, .name = ""});
    }
    std::string name = fn.repr(types());
    if (types().has_type(name))
        return node_type{.type_id = types().get_id(name)};
    fn.name = name;
    unsigned id = types().insert_type(std::move(fn));
    return node_type{.type_id = id};
}

std::optional<node_type> context::make_node_type(node n) {
    assert(n->get_data().is<types::TypeSpec>());

    auto &ts = n->get_data().get<types::TypeSpec>();
    if (!types().has_type(ts.name)) {
        insert_error(
            std::format("type `{}` is not defined", ts.name), n->get_data().loc
        );
        return std::nullopt;
    }

    node_type type{
        .type_id = types().get_id(ts.name), ._is_const = ts.is_const
    };

    bool seen_call = false;
    bool seen_pointer_after_call = false;

    for (auto e : n->forward_edges()) {
        int data = e->get_data().data;
        if (data < types::TypeSpec::DIM0) continue;

        node dim = e->target();
        auto &dim_data = dim->get_data();

        if (dim_data.is<types::Ptr>()) {
            type.insert_pointer(dim_data.get<types::Ptr>().is_const);
            if (seen_call) seen_pointer_after_call = true;
            continue;
        }

        if (dim_data.is<types::Arr>()) {
            if (seen_call && !seen_pointer_after_call) {
                insert_error("array of functions is not allowed", dim_data.loc);
                return std::nullopt;
            }

            node size_node = get_node(dim, types::Arr::SIZE);
            auto value = eval_const(size_node);
            if (!value) return std::nullopt;

            uint64_t count = value->convert_to(8, false).value;
            if (count == 0) {
                insert_error("array size can not be zero", dim_data.loc);
                return std::nullopt;
            }
            if ((int32_t)count < 0) {
                insert_error("array is too large", dim_data.loc);
                return std::nullopt;
            }

            type.insert_array(count);
            continue;
        }

        if (dim_data.is<types::CallSign>()) {
            if (seen_call && !seen_pointer_after_call) {
                insert_error(
                    "a function cannot return a function", dim_data.loc
                );
                return std::nullopt;
            }

            node_type return_type = type;

            std::vector<node_type> params;
            for (auto arg_edge : dim->forward_edges()) {
                if (arg_edge->get_data().data < types::CallSign::ARG0) continue;
                auto arg_type = make_node_type(arg_edge->target());
                if (!arg_type) return std::nullopt;
                if (!validate_type(
                        *arg_type,
                        arg_edge->target()->get_data().loc,
                        "function argument",
                        {.accept_array = false, .accept_void = false}
                    ))
                    return std::nullopt;
                params.push_back(*arg_type);
            }

            type = make_function_type(return_type, params);
            seen_call = true;
            seen_pointer_after_call = false;
            continue;
        }

        assert("invalid type modifier" && false);
    }

    return type;
}

std::optional<types::Literal> context::eval_const(node n) {
    auto &d = n->get_data();

    if (d.is<types::Literal>()) return d.get<types::Literal>();

    if (d.is<types::UnaryOp>()) {
        auto arg = eval_const(get_node(n, types::UnaryOp::ARG));
        if (!arg) return std::nullopt;
        return arg->apply(d.get<types::UnaryOp>().op);
    }

    if (d.is<types::BinaryOp>()) {
        auto l = eval_const(get_node(n, types::BinaryOp::LEFT));
        auto r = eval_const(get_node(n, types::BinaryOp::RIGHT));
        if (!l || !r) return std::nullopt;
        try {
            return l->apply(d.get<types::BinaryOp>().op, *r);
        } catch (std::runtime_error &e) { insert_error(e.what(), d.loc); }
    }

    if (d.is<types::Cast>()) {
        auto value = eval_const(get_node(n, types::Cast::VALUE));
        if (!value) return std::nullopt;

        auto type = make_node_type(get_node(n, types::Cast::TYPE));
        if (!type) return std::nullopt;

        if (!type->is_integer(types()) && !type->is_pointer(types())) {
            insert_error("invalid cast in constant expression", d.loc);
            return std::nullopt;
        }

        const type_info &ti = types().get(type->type_id);
        return types::Literal(
            types::Literal::convert_to(
                value->value,
                value->size,
                value->is_signed,
                ti.size,
                ti.is_signed
            ),
            ti.size,
            ti.is_signed
        );
    }

    if (d.is<types::SizeOf>()) {
        node of = get_node(n, types::SizeOf::OF);
        std::optional<node_type> type;

        if (of->get_data().is<types::TypeSpec>()) {
            if (!(type = make_node_type(of))) return std::nullopt;
        } else {
            if (!check_expr(of)) return std::nullopt;
            assert(of->get_data().type);
            type = *of->get_data().type;
        }

        if (type->is_function(types())) {
            insert_error("sizeof of a function type", d.loc);
            return std::nullopt;
        }

        return types::Literal(type->size_of(types()), 8, false);
    }

    insert_error("non-constant expression in constant context", d.loc);
    return std::nullopt;
}

bool context::is_lvalue(node n, bool require_global) {
    auto &d = n->get_data();

    auto check_if_required = [&require_global](bool cond) {
        return require_global && cond || !require_global;
    };

    if (d.is<types::Declare>()) return true;
    if (d.is<types::ID>()) {
        const std::string &name = d.get<types::ID>().name;
        if (auto *entry = lookup_var(name))
            return check_if_required(entry->is_global);
        return get_function(name) != nullptr;
    }
    if (d.is<types::Deref>())
        return check_if_required(is_lvalue(get_node(n, 0), /* global */ true));
    if (d.is<types::Subscript>())
        return is_lvalue(get_node(n, types::Subscript::VALUE), require_global);
    if (d.is<types::Get>())
        return is_lvalue(get_node(n, types::Get::BASE), require_global);
    if (d.is<types::Cast>())
        return is_lvalue(get_node(n, types::Cast::VALUE), require_global);

    return false;
}

/** check if initializer is valid expression and may be converted to a type */
bool context::check_initializer(node init, const node_type &type) {
    assert(init->get_data().is<types::Initializer>());

    const bool is_global = is_global_scope();
    const bool is_array = type.is_array(types());
    const bool is_struct = type.is_struct(types());
    if (!is_array & !is_struct) {
        return insert_error(
            "list initializer is not allowed for a scalar", init->get_data().loc
        );
    }

    bool ok = true;
    const auto &edges = init->forward_edges();
    node_type ref_type;

    if (is_array) ref_type = type.pointee_type(types());

    for (int i = 0; i < edges.size(); ++i) {
        if (is_struct) {
            const auto &fields = types().get(type.type_id).fields;
            if (i == fields.size()) {
                ok = insert_error(
                    std::format(
                        "too many fields for type `{}`", type.repr(types())
                    ),
                    init->get_data().loc
                );
                continue;
            }
            ref_type = fields[i].type;
        }
        if (is_array && type.mods.back().count == i) {
            ok = insert_error(
                std::format(
                    "too many fields for type `{}`", type.repr(types())
                ),
                init->get_data().loc
            );
            continue;
        }

        node el = edges[i]->target();
        if (!check_expr(el, &ref_type, IGNORE)) {
            ok = false;
            continue;
        }

        if (is_global) {
            if (ref_type.is_pointer(types()) || ref_type.is_integer(types())) {
                if (!eval_const(el)) {
                    ok = false;
                    continue;
                }
            }
        }
    }

    if (ok) init->get_data().type = type;
    return ok;
}

bool context::declare_variable(node decl) {
    const bool is_global = is_global_scope();
    auto &dd = decl->get_data().get<types::Declare>();

    auto type = make_node_type(get_node(decl, types::Declare::TYPE));
    if (!type) return false;

    decl->get_data().type = *type;

    if (!validate_type(
            *type,
            decl->get_data().loc,
            "a variable",
            {.accept_array = true, .accept_void = false}
        ))
        return false;

    // The declared name is not visible in its own initializer, so it is
    // inserted only afterwards initialization check.
    return insert_name(dd.name, *type, is_global, decl->get_data().loc);
}

bool context::check_structure(node n) {
    auto &sd = n->get_data().get<types::Structure>();
    node type_node = get_node(n, types::Structure::TYPE);
    auto &ts = type_node->get_data().get<types::TypeSpec>();

    unsigned type_id;
    if (types().has_type(ts.name)) {
        type_id = types().get_id(ts.name);
    } else {
        type_info info;
        info.name = ts.name;
        info.is_integral = false;
        info.is_function = false;
        info.is_struct = true;
        type_id = types().insert_type(std::move(info));
    }

    // Forward declaration.
    if (!sd.has_body) return true;

    if (types().get(type_id).size != 0) {
        return insert_error(
            std::format("redefinition of type `{}`", ts.name), n->get_data().loc
        );
    }

    // Use types().get(type_id) every time accessing type in this function
    // because every insertion into TypesTable grows the vector and may
    // invalidate reference. However type_id is stable
    bool ok = true;
    for (auto e : n->forward_edges()) {
        if (e->get_data().data < types::Structure::FIELD0) continue;

        node field = e->target();
        auto &fd = field->get_data().get<types::Declare>();

        auto ft = make_node_type(get_node(field, types::Declare::TYPE));
        if (!ft) {
            ok = false;
            continue;
        }

        field->get_data().type = *ft;

        if (!validate_type(
                *ft,
                field->get_data().loc,
                "a struct field",
                {.accept_array = true, .accept_void = false}
            )) {
            ok = false;
            continue;
        }

        if (!types().get(type_id).add_field(fd.name, *ft, types())) {
            insert_error(
                std::format("field `{}` is declared twice", fd.name),
                field->get_data().loc
            );
            ok = false;
        }
    }

    if (!ok) return false;

    types().get(type_id).update_size();

    return ok;
}

bool context::check_expr(node expr, const node_type *as_type, const_policy p) {
    auto &d = expr->get_data();
    bool ok = true;

    if (d.is<types::ID>()) {
        const std::string &name = d.get<types::ID>().name;
        if (auto *entry = lookup_var(name)) {
            d.type = entry->type;
        } else if (auto *fn = get_function(name)) {
            d.type = fn->type;
        } else {
            ok = insert_error(std::format("undefined name `{}`", name), d.loc);
        }
    }

    else if (d.is<types::StringLiteral>()) {
        node_type type{.type_id = types().get_id("int8_t"), ._is_const = true};
        type.insert_pointer(true);
        d.type = type;
    }

    else if (d.is<types::Literal>()) {
        auto &lit = d.get<types::Literal>();
        d.type = make_integer_type(types(), lit.size, lit.is_signed);
    }

    else if (d.is<types::Initializer>()) {
        // initializer list may only be used, if type of it is determined in
        // other context. E.g. initialization or cast
        if (!as_type) {
            ok = insert_error(
                "initizlizer list expression may not be used in this context",
                expr->get_data().loc
            );
        } else {
            ok = check_initializer(expr, *as_type);
        }
    }

    else if (d.is<types::Declare>())
        ok = declare_variable(expr);
    else if (d.is<types::Assign>()) ok = check_assignment(expr);
    else if (d.is<types::BinaryOp>()) ok = check_binary_op(expr);
    else if (d.is<types::UnaryOp>()) ok = check_unary_op(expr);
    else if (d.is<types::Subscript>()) ok = check_subscript(expr);
    else if (d.is<types::Deref>()) ok = check_deref(expr);
    else if (d.is<types::Addr>()) ok = check_addr(expr);
    else if (d.is<types::Get>()) ok = check_get(expr);
    else if (d.is<types::Cast>()) ok = check_cast(expr);
    else if (d.is<types::Call>()) ok = check_call(expr);
    else if (d.is<types::SizeOf>()) ok = check_sizeof(expr);
    else assert("unsupported expression node" && false);

    if (ok && as_type) {
        if (!is_types_compatible(*d.type, *as_type, p)) {
            return insert_error(
                std::format(
                    "invalid cast from `{}` to `{}`",
                    d.type->repr(types()),
                    as_type->repr(types())
                ),
                expr->get_data().loc
            );
        }
    }
    return ok;
}

bool context::check_binary_op(node expr) {
    auto &d = expr->get_data();
    const std::string &op = d.get<types::BinaryOp>().op;

    node left = get_node(expr, types::BinaryOp::LEFT);
    node right = get_node(expr, types::BinaryOp::RIGHT);

    if (!check_expr(left) || !check_expr(right)) return false;

    auto &lt_opt = left->get_data().type;
    auto &rt_opt = right->get_data().type;
    assert(lt_opt);
    assert(rt_opt);

    const node_type &lt = *lt_opt;
    const node_type &rt = *rt_opt;

    const bool li = lt.is_integer(types());
    const bool ri = rt.is_integer(types());
    const bool lp = lt.is_pointer(types());
    const bool rp = rt.is_pointer(types());

    auto fail = [&]() {
        return insert_error(
            std::format(
                "invalid operands `{}` and `{}` to binary operator `{}`",
                lt.repr(types()),
                rt.repr(types()),
                op
            ),
            d.loc
        );
    };

    if (op == "||" || op == "&&") {
        if (!li || !ri) return fail();
        d.type = make_boolean_type(types());
        return true;
    }

    if (op == "==" || op == "!=") {
        if (li && ri) {
            // pass
        } else if (lp && rp) {
            // pass
        } else return fail();
        d.type = make_boolean_type(types());
        return true;
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!li || !ri) return fail();
        d.type = make_boolean_type(types());
        return true;
    }

    if (op == "&" || op == "|" || op == "^") {
        if (!li || !ri) return fail();
        d.type = lt;
        return true;
    }

    if (op == "<<" || op == ">>") {
        if (!li || !ri) return fail();
        d.type = lt;
        return true;
    }

    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        if (!li || !ri) return fail();
        d.type = make_common_integer_type(types(), lt, rt);
        return true;
    }

    return fail();
}

bool context::check_unary_op(node expr) {
    auto &d = expr->get_data();
    const std::string &op = d.get<types::UnaryOp>().op;

    node arg = get_node(expr, types::UnaryOp::ARG);
    if (!check_expr(arg)) return false;

    auto &arg_type = arg->get_data().type;
    assert(arg_type);

    if (!arg_type->is_integer(types())) {
        return insert_error(
            std::format(
                "invalid operand of type `{}` to unary operator `{}`",
                arg_type->repr(types()),
                op
            ),
            d.loc
        );
    }

    if (op == "!") d.type = make_boolean_type(types());
    else d.type = *arg_type;
    return true;
}

bool context::check_subscript(node expr) {
    auto &d = expr->get_data();

    node value = get_node(expr, types::Subscript::VALUE);
    node index = get_node(expr, types::Subscript::IDX);

    if (!check_expr(value)) return false;
    if (!check_expr(index)) return false;

    auto &value_type = value->get_data().type;
    auto &index_type = index->get_data().type;
    assert(value_type);
    assert(index_type);

    if (!value_type->is_array(types()) && !value_type->is_pointer(types())) {
        return insert_error(
            "subscripted value is not an array or a pointer", d.loc
        );
    }

    if (!index_type->is_integer(types())) {
        return insert_error("subscript index is not an integer", d.loc);
    }

    d.type = value_type->pointee_type(types());
    return true;
}

bool context::check_deref(node expr) {
    auto &d = expr->get_data();

    node value = get_node(expr, 0);
    if (!check_expr(value)) return false;

    auto &value_type = value->get_data().type;
    assert(value_type);

    if (!value_type->is_pointer(types())) {
        return insert_error("cannot dereference a non-pointer value", d.loc);
    }

    d.type = value_type->pointee_type(types());
    return true;
}

bool context::check_addr(node expr) {
    auto &d = expr->get_data();

    node value = get_node(expr, 0);
    if (!check_expr(value)) return false;

    auto &value_type = value->get_data().type;
    assert(value_type);

    if (!is_lvalue(value)) {
        return insert_error(
            "cannot take the address of a non-lvalue expression", d.loc
        );
    }

    d.type = *value_type;
    d.type->insert_pointer(false);
    return true;
}

bool context::check_get(node expr) {
    auto &d = expr->get_data();
    const std::string &member = d.get<types::Get>().name;

    node base = get_node(expr, types::Get::BASE);
    if (!check_expr(base)) return false;

    auto &base_type = base->get_data().type;
    assert(base_type);

    if (!base_type->is_struct(types())) {
        return insert_error(
            std::format(
                "member access on a non-struct value of type `{}`",
                base_type->repr(types())
            ),
            d.loc
        );
    }

    const type_info &info = types().get(base_type->type_id);
    auto field = std::ranges::find_if(info.fields, [&member](auto &f) {
        return f.name == member;
    });
    if (field == info.fields.end()) {
        return insert_error(
            std::format(
                "type `{}` has no member named `{}`",
                base_type->repr(types()),
                member
            ),
            d.loc
        );
    }

    d.type = field->type;
    if (base_type->is_const(types())) d.type = d.type->as_const(types());
    return true;
}

bool context::check_cast(node expr) {
    auto &d = expr->get_data();
    const static unsigned void_type_id = types().get_id("void_t");

    auto type = make_node_type(get_node(expr, types::Cast::TYPE));
    if (!type) return false;

    node value = get_node(expr, types::Cast::VALUE);
    if (value->get_data().is<types::Initializer>()) {
        if (!check_expr(value, &*type, IGNORE)) return false;
    } else if (!check_expr(value)) return false;

    const auto &value_type = value->get_data().type;
    assert(value_type);

    const bool a_int = type->is_integer(types());
    const bool a_ptr = type->is_pointer(types());
    const bool a_arr = type->is_array(types());
    const bool b_int = value_type->is_integer(types());
    const bool b_ptr = value_type->is_pointer(types());
    const bool b_arr = value_type->is_array(types());

    // cast must be only to a compatible type
    // only exceptions are casts:
    // * int -> ptr
    // * arr -> ptr
    // * ptr -> int
    // * int -> int
    if (a_ptr && (b_int || b_arr)) goto ok;
    if (a_int && (b_int || b_ptr)) goto ok;

    // also allow cast from one array into another with the same common size
    if (a_arr && b_arr && type->size_of(types()) == type->size_of(types()))
        goto ok;

    if (!is_types_compatible(*type, *value_type, IGNORE))
        return insert_error(
            std::format(
                "invalid cast from `{}` to `{}`",
                value_type->repr(types()),
                type->repr(types())
            ),
            expr->get_data().loc
        );

ok:
    d.type = type;
    return true;
}

bool context::check_call(node expr) {
    auto &d = expr->get_data();

    node callee = get_node(expr, types::Call::EXPR);
    if (!check_expr(callee)) return false;

    auto &callee_type = callee->get_data().type;
    assert(callee_type);

    if (!callee_type->is_function(types())) {
        return insert_error(
            std::format(
                "attempt to call non-function type `{}`",
                callee_type->repr(types())
            ),
            d.loc
        );
    }

    const type_info &fn = types().get(callee_type->type_id);

    std::vector<node> args;
    for (auto e : expr->forward_edges()) {
        if (e->get_data().data < types::Call::ARG0) continue;
        args.push_back(e->target());
    }

    if (args.size() != fn.fields.size()) {
        return insert_error(
            std::format(
                "function expects {} argument(s), got {}",
                fn.fields.size(),
                args.size()
            ),
            d.loc
        );
    }

    bool ok = true;
    for (size_t i = 0; i < args.size(); ++i) {
        node arg = args[i];
        if (!check_expr(arg)) {
            ok = false;
            continue;
        }
        auto &arg_type = arg->get_data().type;
        assert(arg_type);
        if (!is_types_compatible(fn.fields[i].type, *arg_type, MAY_BE_CONST)) {
            ok = insert_error(
                std::format(
                    "argument {} has type `{}`, expected `{}`",
                    i,
                    arg_type->repr(types()),
                    fn.fields[i].type.repr(types())
                ),
                arg->get_data().loc
            );
        }
    }

    if (!ok) return false;

    d.type = fn.return_type;
    return true;
}

bool context::check_sizeof(node expr) {
    auto &d = expr->get_data();
    node of = get_node(expr, types::SizeOf::OF);
    std::optional<node_type> type;

    if (of->get_data().is<types::TypeSpec>()) {
        type = make_node_type(of);
        if (!type) return false;
    } else {
        if (!check_expr(of)) return false;
        type = of->get_data().type;
        assert(type);
    }

    if (type->is_function(types())) {
        return insert_error("sizeof of a function type is not allowed", d.loc);
    } else if (type->size_of(types()) == 0) {
        return insert_error(
            "sizeof of an incomplete type is not allowed", d.loc
        );
    }

    d.type = make_integer_type(types(), 8, false);
    return true;
}

bool context::check_condition(node cond) {
    if (!check_expr(cond)) return false;
    auto &type = cond->get_data().type;
    assert(type);

    if (!type->is_integer(types()))
        return insert_error(
            "condition must have an integer type", cond->get_data().loc
        );
    return true;
}

bool context::check_return(node stmt) {
    if (!current_function) {
        return insert_error(
            "return statement outside of a function", stmt->get_data().loc
        );
    }

    const node_type &ret = current_function->type.return_type(types());
    const bool is_void = ret.is_void(types());

    auto it = find_edge(stmt, 0);
    if (it == stmt->forward_edges().end()) {
        if (!is_void)
            return insert_error(
                "a non-void function must return a value", stmt->get_data().loc
            );
        return true;
    }

    if (is_void) {
        return insert_error(
            "a void function cannot return a value", stmt->get_data().loc
        );
    }

    node value = (*it)->target();

    if (!check_expr(value)) return false;
    auto &value_type = value->get_data().type;
    assert(value_type);

    if (!is_types_compatible(ret, *value_type, MAY_BE_CONST))
        return insert_error(
            std::format(
                "type `{}` is not compatible with return-type `{}`",
                value_type->repr(types()),
                ret.repr(types())
            ),
            value->get_data().loc
        );
    return true;
}

bool context::check_assignment(node expr) {
    auto &d = expr->get_data();

    node right = get_node(expr, types::Assign::RIGHT);
    node left = get_node(expr, types::Assign::LEFT);

    const bool is_declare = left->get_data().is<types::Declare>();

    // if left is declaration, determine it's type first to correctly process
    // right type, if needed
    if (is_declare) {
        node l_type_n = get_node(left, types::Declare::TYPE);
        auto left_type = make_node_type(l_type_n);
        if (!left_type) return false;

        if (!check_expr(right, &*left_type, IGNORE)) return false;
    } else if (!check_expr(right)) return false;

    if (is_global_scope()) {
        // if right is initizlizer -- it was already checked for globalness
        if (!right->get_data().is<types::Initializer>() && !eval_const(right)) {
            return insert_error(
                "Non-constatnt expression in global context",
                right->get_data().loc
            );
        }
    }

    const auto &right_type = right->get_data().type;
    assert(right_type);

    if (!check_expr(left)) return false;
    const auto &left_type = left->get_data().type;
    assert(left_type);

    if (!is_lvalue(left)) {
        return insert_error(
            "left hand side of assignment is not an lvalue", d.loc
        );
    }

    if (!is_declare && left_type->is_const(types())) {
        return insert_error(
            std::format(
                "object of type `{}` is not assignable",
                left_type->repr(types())
            ),
            d.loc
        );
    }

    if (!is_types_compatible(*left_type, *right_type, IGNORE)) {
        return insert_error(
            std::format(
                "invalid assignment of type `{}` into type `{}`",
                right_type->repr(types()),
                left_type->repr(types())
            ),
            right->get_data().loc
        );
    }

    return true;
}

bool context::check_stmt(node stmt) {
    auto &d = stmt->get_data();

    if (d.is<types::Block>()) return check_block(stmt, true);

    if (d.is<types::Declare>()) return declare_variable(stmt);
    if (d.is<types::Assign>()) return check_assignment(stmt);
    if (d.is<types::Return>()) return check_return(stmt);
    if (d.is<types::If>()) return check_if_stmt(stmt);
    if (d.is<types::For>()) return check_for_stmt(stmt);

    if (d.is<types::ForInit>() || d.is<types::ForStep>()) {
        bool ok = true;
        for (auto e : stmt->forward_edges()) ok = check_expr(e->target()) && ok;
        return ok;
    }

    const bool loop = is_in_loop();

    if (d.is<types::Break>())
        return loop || insert_error("break outside of a loop", d.loc);

    if (d.is<types::Continue>())
        return loop || insert_error("continue outside of a loop", d.loc);

    return check_expr(stmt);
}

bool context::check_for_stmt(node stmt) {
    bool ok = true;

    push_scope(true);

    // init stmt is being processed before others to declare local scope names
    node init = get_node(stmt, types::For::INIT);
    if (!check_stmt(init)) return false;

    for (auto e : stmt->forward_edges()) {
        switch (e->get_data().data) {
        case types::For::INIT: break;
        case types::For::STEP: ok = check_stmt(e->target()) && ok; break;
        case types::For::COND: ok = check_condition(e->target()) && ok; break;
        case types::For::STMT: ok = check_stmt(e->target()) && ok; break;
        default: assert("invalid node type" && false);
        }
    }

    pop_scope();
    return ok;
}

bool context::check_if_stmt(node stmt) {
    auto cond = get_node(stmt, types::If::COND);
    if (!check_condition(cond)) return false;

    auto then = get_node(stmt, types::If::IF_STMT);
    if (!check_stmt(then)) return false;

    auto else_e = find_edge(stmt, types::If::ELSE_STMT);
    if (else_e != stmt->forward_edges().end())
        if (!check_stmt((*else_e)->target())) return false;

    return true;
}

bool context::check_block(node blk, bool make_scope) {
    if (make_scope) push_scope(false);

    bool ok = true;
    for (auto e : blk->forward_edges()) {
        if (!check_stmt(e->target())) {
            ok = false;
            break;
        }
    }

    if (make_scope) pop_scope();
    return ok;
}

bool context::check_function(node stmt) {
    node decl = get_node(stmt, types::Function::DECL);
    const std::string &name = decl->get_data().get<types::Declare>().name;

    // validate return type
    auto return_type = make_node_type(get_node(decl, types::Declare::TYPE));
    if (!return_type) return false;
    if (!validate_type(
            *return_type,
            decl->get_data().loc,
            "a return type",
            {
                .accept_array = true,
                .accept_void = true,
            }
        ))
        return false;

    // collect and validate types of arguments to determine function type before
    // any declarations
    bool ok = true;
    std::vector<node_type> arg_types;
    for (auto e : stmt->forward_edges()) {
        if (e->get_data().data < types::Function::ARG0) continue;
        node arg = e->target();
        auto arg_type = make_node_type(get_node(arg, types::Declare::TYPE));
        if (!arg_type) {
            ok = false;
            continue;
        }

        if (!validate_type(
                *arg_type,
                arg->get_data().loc,
                "a function argumnet",
                {.accept_array = false, .accept_void = false}
            )) {
            ok = false;
            continue;
        }

        arg_types.push_back(*arg_type);
    }
    if (!ok) return false;

    // build function type from return type and argumnets
    node_type fn_type = make_function_type(*return_type, arg_types);
    decl->get_data().type = fn_type;

    // determine, wether body of function is defined for this stetement
    auto body_edge = find_edge(stmt, types::Function::BODY);
    const bool has_body = body_edge != stmt->forward_edges().end();

    // check if function with such name was declared earlier
    function_entry *fn = nullptr;
    if ((fn = get_function(name))) {
        // if declared, check if type is strictly same
        if (!is_types_compatible(fn->type, fn_type, STRICT)) {
            return insert_error(
                std::format("conflicting declaration of function `{}`", name),
                decl->get_data().loc
            );
        }

        // if current statement is defined with body then previous declaration
        // must be a prototype
        if (has_body && fn->body) {
            return insert_error(
                std::format("redefinition of function `{}`", name),
                decl->get_data().loc
            );
        }
    } else if (!(fn = register_function(name, fn_type, stmt->get_data().loc))) {
        return false;
    }

    if (has_body) {
        fn->body = (*body_edge)->target();
        current_function = fn;
        push_scope(false);

        for (edge e : stmt->forward_edges()) {
            if (e->get_data().data < types::Function::ARG0) continue;
            if (!declare_variable(e->target())) ok = false;
        }

        if (!ok) return false;
        if (!check_block(*fn->body, false)) return false;

        pop_scope();
        current_function = nullptr;
    }

    return ok;
}

bool context::check_main() {
    function_entry *main_fn = get_function("main");
    if (!main_fn) return insert_error("function `main` is required", {});

    if (!main_fn->body)
        return insert_error("function `main` must be defined", {});

    node_type i32 = make_integer_type(types(), 4, true);
    node_type u32 = make_integer_type(types(), 4, false);
    node_type u8_const{.type_id = types().get_id("uint8_t"), ._is_const = true};
    u8_const.insert_pointer(false);
    u8_const.insert_pointer(false);

    const type_info &sig = types().get(main_fn->type.type_id);

    bool valid =
        is_types_compatible(main_fn->type.return_type(types()), i32, IGNORE);
    if (sig.fields.size() == 2) {
        valid |= is_types_compatible(sig.fields[0].type, u32, IGNORE)
              && is_types_compatible(sig.fields[1].type, u8_const, IGNORE);
    } else if (!sig.fields.empty()) valid = false;

    if (!valid)
        return insert_error(
            "invalid signature of function `main`", main_fn->loc
        );

    return true;
}

bool context::check_global_names() {
    Ast &ast = pctx.ast;
    node root = ast.get_root();
    assert(root->get_data().is<types::Root>());

    push_scope(false);

    bool ok = true;
    for (auto e : root->forward_edges()) {
        node stmt = e->target();
        auto &d = stmt->get_data();

        if (d.is<types::Structure>()) {
            ok = check_structure(stmt) && ok;
        } else if (d.is<types::Function>()) {
            ok = check_function(stmt) && ok;
        } else if (d.is<types::Declare>()) {
            ok = declare_variable(stmt) && ok;
        } else if (d.is<types::Assign>()) {
            ok = check_assignment(stmt) && ok;
        } else {
            assert("invalid global statement" && false);
        }
    }

    ok = check_main() && ok;

    return ok && pctx.errors.empty();
}

bool Ast::check_semantics(parser::ParserContext &ctx) {
    context c{.pctx = ctx};
    return c.check_global_names();
}

}; // namespace frontend::ast
