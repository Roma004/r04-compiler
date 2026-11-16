#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "contract/hir/hir.hpp"
#include "contract/hir/operations.hpp"
#include "frontend/ast/ast.hpp"

namespace frontend::ast {

using contract::hir::object_type;
using contract::hir::TypeTable;
namespace hir = contract::hir;
namespace ops = contract::hir::ops;
using hir::Value;

using node_iter = Ast::node_iter;

constexpr unsigned kInvalidObj = std::numeric_limits<unsigned>::max();

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

template <typename OpT> OpT make_binop(Value dst, Value lhs, Value rhs) {
    OpT op;
    op.dst = dst;
    op.lhs = lhs;
    op.rhs = rhs;
    return op;
}

template <typename OpT> OpT make_unop(Value dst, Value src) {
    OpT op;
    op.dst = dst;
    op.src = src;
    return op;
}

object_type ptr_to(object_type type) {
    type.insert_pointer(false);
    return type;
}

struct HirBuilder {

    using node = Ast::node_iter;
    using edge = Ast::edge_iter;

    hir::TranslationUnit &unit;
    TypeTable &types;
    hir::Function *fn = nullptr;

    std::vector<std::unordered_map<std::string, unsigned>> scopes;
    std::vector<unsigned> break_stack;
    std::vector<unsigned> continue_stack;

    unsigned next_label = 0;
    unsigned next_temp = 0;
    unsigned next_global = 0;

    void build(Ast &ast) {
        for (auto e : ast.get_root()->forward_edges()) {
            node stmt = e->target();
            if (stmt->get_data().is<types::Function>()) build_function(stmt);
            else if (stmt->get_data().is<types::Declare>())
                build_global_declare(stmt);
        }
    }

    Value make_temp(object_type ty) {
        hir::Object o{
            .name = "__t" + std::to_string(next_temp++),
            .type = std::move(ty),
            .constant_id = 0,
            .is_automatic = true,
        };
        auto id = unit.add_object(std::move(o));
        if (fn) fn->locals.push_back(id);
        return id;
    }

    Value make_local(std::string_view name, object_type type) {
        hir::Object o{
            .name = std::string(name),
            .type = std::move(type),
            .constant_id = 0,
        };
        auto id = unit.add_object(std::move(o));
        if (fn) fn->locals.push_back(id);
        return id;
    }

    Value make_global(std::string_view name, object_type type) {
        hir::Object o{
            .name = std::string(name),
            .type = std::move(type),
            .constant_id = 0,
            .is_global = true,
        };
        return unit.add_object(std::move(o));
    }

    Value make_label() { return Value::lbl(next_label++); }

    template <typename OpT> void make_op(OpT op) {
        hir::Operation o{.op = std::move(op)};
        o.check("op#" + std::to_string(fn->ops.size()));
        fn->ops.push_back(std::move(o));
    }

    Value make_const(uint64_t val, object_type type) {
        Value obj = make_temp(type);
        make_op<ops::Const>(
            {.dst = obj,
             .value =
                 unit.add_constant(hir::Constant{.type = type, .value = val})}
        );
        return obj;
    }

    Value make_int_constant(uint64_t val) {
        return unit.add_constant({
            .type = types.get("uint64_t").make_type(),
            .value = val,
        });
    }

    std::optional<unsigned> lookup(std::string_view name) {
        std::string key(name);
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(key);
            if (f != it->end()) return f->second;
        }
        return std::nullopt;
    }

    void push_scope() { scopes.emplace_back(); }
    void pop_scope() { scopes.pop_back(); }
    void declare_local(std::string_view name, unsigned oid) {
        scopes.back()[std::string(name)] = oid;
    }

    // ---------- глобальные ----------

    void build_global_declare(node_iter n) {
        auto &decl = n->get<types::Declare>();
        new_global(decl.name, n->type.value());
    }

    // ---------- функции ----------

    void build_function(node_iter n) {
        auto decl_node = child(n, types::Function::DECL).value();
        auto &decl = decl_node->get<types::Declare>();

        hir::Function f;
        f.name = decl.name;
        f.return_type = decl_node->type.value();
        f.address_obj = kInvalidObj; // не выделяли
        f.is_external = !has_child(n, types::Function::BODY);

        auto fid = tu.add_function(std::move(f));
        fn = &tu.functions[fid];

        scopes.clear();
        break_stack.clear();
        continue_stack.clear();
        push_scope(); // глобальная область
        push_scope(); // область функции

        for (auto a : children_from(n, types::Function::ARG0)) {
            auto &arg = a->get<types::Declare>();
            object_type ty = a->type.value();
            auto oid = new_local(arg.name, ty);
            fn->params.push_back(oid);
            declare_local(arg.name, oid);
        }

        if (auto body = child(n, types::Function::BODY))
            lower_stmt(body.value());

        fn = nullptr;
        scopes.clear();
    }

    // ---------- операторы ----------

    void lower_stmt(node_iter n) {
        if (n->is<types::Block>()) {
            push_scope();
            for (auto s : children_from(n, types::Block::STMT0)) lower_stmt(s);
            pop_scope();
        } else if (n->is<types::Declare>()) {
            auto &decl = n->get<types::Declare>();
            auto oid = new_local(decl.name, n->type.value());
            declare_local(decl.name, oid);
        } else if (n->is<types::Assign>()) {
            lower_assign(n);
        } else if (n->is<types::If>()) {
            lower_if(n);
        } else if (n->is<types::For>()) {
            lower_for(n);
        } else if (n->is<types::ForInit>()) {
            for (auto s : children_from(n, types::ForInit::STMT0))
                lower_stmt(s);
        } else if (n->is<types::ForStep>()) {
            for (auto s : children_from(n, types::ForStep::STMT0))
                lower_stmt(s);
        } else if (n->is<types::Return>()) {
            lower_return(n);
        } else if (n->is<types::Break>()) {
            if (!break_stack.empty()) {
                ops::Jump j;
                j.label = Value::lbl(break_stack.back());
                emit(std::move(j));
            }
        } else if (n->is<types::Continue>()) {
            if (!continue_stack.empty()) {
                ops::Jump j;
                j.label = Value::lbl(continue_stack.back());
                emit(std::move(j));
            }
        } else if (
            n->is<types::Structure>() || n->is<types::TypeSpec>()
            || n->is<types::Ptr>() || n->is<types::Arr>()
            || n->is<types::CallSign>()
        ) {
            // чистые описания типов — ничего не эмитим
        } else {
            lower_expr(n);
        }
    }

    void lower_if(node_iter n) {
        auto cond_node = child(n, types::If::COND).value();
        auto then_node = child(n, types::If::IF_STMT).value();
        auto else_node = child(n, types::If::ELSE_STMT);

        auto cond = lower_expr(cond_node);

        unsigned l_then = new_label();
        unsigned l_else = new_label();
        unsigned l_end = new_label();

        ops::Branch br;
        br.cond = cond;
        br.true_label = Value::lbl(l_then);
        br.false_label = Value::lbl(else_node ? l_else : l_end);
        emit(std::move(br));

        ops::Label lt;
        lt.label = Value::lbl(l_then);
        emit(std::move(lt));

        lower_stmt(then_node);

        ops::Jump j1;
        j1.label = Value::lbl(l_end);
        emit(std::move(j1));

        if (else_node) {
            ops::Label le;
            le.label = Value::lbl(l_else);
            emit(std::move(le));

            lower_stmt(else_node.value());

            ops::Jump j2;
            j2.label = Value::lbl(l_end);
            emit(std::move(j2));
        }

        ops::Label lend;
        lendl.label = Value::lbl(l_end);
        emit(std::move(lendl));
    }

    void lower_for(node_iter n) {
        push_scope();

        if (auto init = child(n, types::For::INIT)) lower_stmt(init.value());

        unsigned l_cond = new_label();
        unsigned l_body = new_label();
        unsigned l_step = new_label();
        unsigned l_end = new_label();

        ops::Label lc;
        lc.label = Value::lbl(l_cond);
        emit(std::move(lc));

        if (auto cond_node = child(n, types::For::COND)) {
            auto cond = lower_expr(cond_node.value());
            ops::Branch br;
            br.cond = cond;
            br.true_label = Value::lbl(l_body);
            br.false_label = Value::lbl(l_end);
            emit(std::move(br));
        } else {
            ops::Jump j;
            j.label = Value::lbl(l_body);
            emit(std::move(j));
        }

        ops::Label lb;
        lb.label = Value::lbl(l_body);
        emit(std::move(lb));

        break_stack.push_back(l_end);
        continue_stack.push_back(l_step);

        if (auto body = child(n, types::For::STMT)) lower_stmt(body.value());

        break_stack.pop_back();
        continue_stack.pop_back();

        ops::Label ls;
        ls.label = Value::lbl(l_step);
        emit(std::move(ls));

        if (auto step = child(n, types::For::STEP)) lower_stmt(step.value());

        ops::Jump jb;
        jb.label = Value::lbl(l_cond);
        emit(std::move(jb));

        ops::Label lend;
        lend.label = Value::lbl(l_end);
        emit(std::move(lend));

        pop_scope();
    }

    void lower_return(node_iter n) {
        auto ch = children_from(n, 0);
        ops::Return op;
        if (!ch.empty()) op.varargs.push_back(lower_expr(ch[0]));
        emit(std::move(op));
    }

    void lower_assign(node_iter n) {
        auto &assign = n->get<types::Assign>();
        auto lhs_node = child(n, types::Assign::LEFT).value();
        auto rhs_node = child(n, types::Assign::RIGHT).value();

        if (rhs_node->is<types::Initializer>()) {
            lower_init_list(lhs_node, rhs_node);
            return;
        }

        auto rhs = lower_expr(rhs_node);
        auto lhs_ptr = lower_lvalue(lhs_node);

        if (assign.op.empty()) {
            ops::Store st;
            st.ptr = lhs_ptr;
            st.value = rhs;
            emit(std::move(st));
        } else {
            // составное присваивание: x op= y
            auto base_op = assign.op.substr(0, assign.op.size() - 1);
            object_type ty = lhs_node->type.value();

            auto old = new_temp(ty);
            ops::Load ld;
            ld.dst = Value::obj(old);
            ld.ptr = lhs_ptr;
            emit(std::move(ld));

            auto res = new_temp(ty);
            emit_binop(base_op, Value::obj(res), Value::obj(old), rhs);

            ops::Store st;
            st.ptr = lhs_ptr;
            st.value = Value::obj(res);
            emit(std::move(st));
        }
    }

    void emit_binop(std::string_view op, Value dst, Value lhs, Value rhs) {
        if (op == "+") emit(make_binop<ops::Add>(dst, lhs, rhs));
        else if (op == "-") emit(make_binop<ops::Sub>(dst, lhs, rhs));
        else if (op == "*") emit(make_binop<ops::Mul>(dst, lhs, rhs));
        else if (op == "/") emit(make_binop<ops::Div>(dst, lhs, rhs));
        else if (op == "%") emit(make_binop<ops::Mod>(dst, lhs, rhs));
        else if (op == "&") emit(make_binop<ops::And>(dst, lhs, rhs));
        else if (op == "|") emit(make_binop<ops::Or>(dst, lhs, rhs));
        else if (op == "^") emit(make_binop<ops::Xor>(dst, lhs, rhs));
        else if (op == "<<") emit(make_binop<ops::Shl>(dst, lhs, rhs));
        else if (op == ">>") emit(make_binop<ops::Shr>(dst, lhs, rhs));
        else if (op == "==") emit(make_binop<ops::Eq>(dst, lhs, rhs));
        else if (op == "!=") emit(make_binop<ops::Ne>(dst, lhs, rhs));
        else if (op == "<") emit(make_binop<ops::Lt>(dst, lhs, rhs));
        else if (op == "<=") emit(make_binop<ops::Le>(dst, lhs, rhs));
        else if (op == ">") emit(make_binop<ops::Gt>(dst, lhs, rhs));
        else if (op == ">=") emit(make_binop<ops::Ge>(dst, lhs, rhs));
        else
            throw std::runtime_error(
                "unsupported binary operator: " + std::string(op)
            );
    }

    void lower_init_list(node_iter lhs_node, node_iter init) {
        auto lhs_ptr = lower_lvalue(lhs_node);
        int idx = 0;
        for (auto elem : children_from(init, 0)) {
            auto val = lower_expr(elem);
            object_type et = elem->type.value();
            auto idx_obj = materialize_const(idx, int_type());
            auto elem_ptr = new_temp(ptr_to(et));

            ops::ElemPtr ep;
            ep.dst = Value::obj(elem_ptr);
            ep.arr = lhs_ptr;
            ep.idx = idx_obj;
            emit(std::move(ep));

            ops::Store st;
            st.ptr = Value::obj(elem_ptr);
            st.value = val;
            emit(std::move(st));
            ++idx;
        }
    }

    // ---------- lvalue ----------

    Value lower_lvalue(node_iter n) {
        if (n->is<types::ID>()) {
            auto &id = n->get<types::ID>();
            if (tu.find_function_by_name(id.name))
                throw std::runtime_error(
                    "taking address of function is not yet supported: "
                    + id.name
                );

            auto obj = lookup(id.name);
            if (!obj) {
                auto *g = tu.find_global_by_name(id.name);
                if (!g) throw std::runtime_error("undefined: " + id.name);
                obj = g->id;
            }
            object_type ty = n->type.value();
            auto ptr = new_temp(ptr_to(ty));

            ops::AddrOf ao;
            ao.dst = Value::obj(ptr);
            ao.src = Value::obj(*obj);
            emit(std::move(ao));
            return Value::obj(ptr);
        }
        if (n->is<types::Deref>()) {
            return lower_expr(child(n, types::Deref::BASE).value());
        }
        if (n->is<types::Subscript>()) {
            auto base = lower_expr(child(n, types::Subscript::VALUE).value());
            auto idx = lower_expr(child(n, types::Subscript::IDX).value());
            object_type et = n->type.value();
            auto ptr = new_temp(ptr_to(et));

            ops::ElemPtr ep;
            ep.dst = Value::obj(ptr);
            ep.arr = base;
            ep.idx = idx;
            emit(std::move(ep));
            return Value::obj(ptr);
        }
        if (n->is<types::Get>()) {
            auto base = child(n, types::Get::BASE).value();
            auto &g = n->get<types::Get>();
            auto base_ptr = lower_lvalue(base);

            unsigned fidx = lookup_field_index(base->type.value(), g.name);
            object_type ft = n->type.value();
            auto ptr = new_temp(ptr_to(ft));

            auto idx_const = make_int_constant(fidx);

            ops::FieldPtr fp;
            fp.dst = Value::obj(ptr);
            fp.ptr = base_ptr;
            fp.field_idx = Value::cst(idx_const);
            emit(std::move(fp));
            return Value::obj(ptr);
        }
        throw std::runtime_error("not an lvalue: " + n->repr(tt));
    }

    unsigned lookup_field_index(object_type ty, std::string_view name) {
        while (ty.is_pointer(tt)) ty = ty.pointee_type(tt);
        if (!ty.is_struct(tt))
            throw std::runtime_error(
                "field access on non-struct: " + std::string(name)
            );
        auto &info = tt.get(ty.type_id);
        for (size_t i = 0; i < info.fields.size(); ++i)
            if (info.fields[i].name == name) return static_cast<unsigned>(i);
        throw std::runtime_error("unknown field: " + std::string(name));
    }

    // ---------- выражения ----------

    Value lower_expr(node_iter n) {
        if (n->is<types::Literal>()) {
            auto &lit = n->get<types::Literal>();
            return materialize_const(lit.value, n->type.value());
        }
        if (n->is<types::StringLiteral>()) return lower_string(n);
        if (n->is<types::ID>()) return lower_id(n);
        if (n->is<types::BinaryOp>()) return lower_binop(n);
        if (n->is<types::UnaryOp>()) return lower_unop(n);

        if (n->is<types::Addr>()) {
            auto ch = children_from(n, 0);
            if (ch.empty()) throw std::runtime_error("Addr: no operand");
            return lower_lvalue(ch[0]);
        }
        if (n->is<types::Deref>()) {
            auto ptr = lower_expr(child(n, types::Deref::BASE).value());
            object_type ty = n->type.value();
            auto dst = new_temp(ty);

            ops::Load ld;
            ld.dst = Value::obj(dst);
            ld.ptr = ptr;
            emit(std::move(ld));
            return Value::obj(dst);
        }
        if (n->is<types::Subscript>() || n->is<types::Get>()) {
            object_type ty = n->type.value();
            auto ptr = lower_lvalue(n);
            if (is_array_ty(ty)) return ptr;

            auto dst = new_temp(ty);
            ops::Load ld;
            ld.dst = Value::obj(dst);
            ld.ptr = ptr;
            emit(std::move(ld));
            return Value::obj(dst);
        }
        if (n->is<types::Cast>()) {
            auto src = lower_expr(child(n, types::Cast::VALUE).value());
            object_type ty = n->type.value();
            auto dst = new_temp(ty);

            ops::Cast op;
            op.dst = Value::obj(dst);
            op.src = src;
            emit(std::move(op));
            return Value::obj(dst);
        }
        if (n->is<types::Call>()) return lower_call(n);
        if (n->is<types::SizeOf>()) return lower_sizeof(n);

        if (n->is<types::Declare>()) {
            auto &decl = n->get<types::Declare>();
            auto oid = new_local(decl.name, n->type.value());
            declare_local(decl.name, oid);
            return Value::obj(oid);
        }

        if (n->is<types::Assign>()) {
            lower_assign(n);
            return lower_expr(child(n, types::Assign::RIGHT).value());
        }

        throw std::runtime_error("cannot lower expression: " + n->repr(tt));
    }

    Value lower_id(node_iter n) {
        auto &id = n->get<types::ID>();
        if (auto *f = tu.find_function_by_name(id.name))
            return Value::fn(f->id);

        auto obj = lookup(id.name);
        if (!obj) {
            auto *g = tu.find_global_by_name(id.name);
            if (!g)
                throw std::runtime_error("undefined identifier: " + id.name);
            obj = g->id;
        }

        object_type ty = n->type.value();
        if (is_array_ty(ty)) return Value::obj(*obj);

        auto ptr = new_temp(ptr_to(ty));
        ops::AddrOf ao;
        ao.dst = Value::obj(ptr);
        ao.src = Value::obj(*obj);
        emit(std::move(ao));

        auto dst = new_temp(ty);
        ops::Load ld;
        ld.dst = Value::obj(dst);
        ld.ptr = Value::obj(ptr);
        emit(std::move(ld));
        return Value::obj(dst);
    }

    Value lower_binop(node_iter n) {
        auto &bop = n->get<types::BinaryOp>();
        if (bop.op == "&&") return lower_logical_and(n);
        if (bop.op == "||") return lower_logical_or(n);

        auto lhs = lower_expr(child(n, types::BinaryOp::LEFT).value());
        auto rhs = lower_expr(child(n, types::BinaryOp::RIGHT).value());
        object_type ty = n->type.value();
        auto dst = new_temp(ty);
        emit_binop(bop.op, Value::obj(dst), lhs, rhs);
        return Value::obj(dst);
    }

    Value lower_unop(node_iter n) {
        auto &uop = n->get<types::UnaryOp>();
        auto arg = lower_expr(child(n, types::UnaryOp::ARG).value());
        if (uop.op == "+") return arg;

        object_type ty = n->type.value();
        auto dst = new_temp(ty);

        if (uop.op == "-") emit(make_unop<ops::Neg>(Value::obj(dst), arg));
        else if (uop.op == "!") emit(make_unop<ops::Not>(Value::obj(dst), arg));
        else if (uop.op == "~") emit(make_unop<ops::Inv>(Value::obj(dst), arg));
        else throw std::runtime_error("unsupported unary operator: " + uop.op);

        return Value::obj(dst);
    }

    Value lower_logical_and(node_iter n) {
        object_type ty = n->type.value();
        auto result = new_temp(ty);
        auto zero = materialize_const(0, ty);
        auto rptr = new_temp(ptr_to(ty));

        ops::AddrOf ao;
        ao.dst = Value::obj(rptr);
        ao.src = Value::obj(result);
        emit(std::move(ao));

        ops::Store st0;
        st0.ptr = Value::obj(rptr);
        st0.value = zero;
        emit(std::move(st0));

        unsigned l_true = new_label();
        unsigned l_end = new_label();

        auto lhs = lower_expr(child(n, types::BinaryOp::LEFT).value());

        ops::Branch br;
        br.cond = lhs;
        br.true_label = Value::lbl(l_true);
        br.false_label = Value::lbl(l_end);
        emit(std::move(br));

        ops::Label lt;
        lt.label = Value::lbl(l_true);
        emit(std::move(lt));

        auto rhs = lower_expr(child(n, types::BinaryOp::RIGHT).value());

        ops::Store st1;
        st1.ptr = Value::obj(rptr);
        st1.value = rhs;
        emit(std::move(st1));

        ops::Label le;
        le.label = Value::lbl(l_end);
        emit(std::move(le));

        return Value::obj(result);
    }

    Value lower_logical_or(node_iter n) {
        object_type ty = n->type.value();
        auto result = new_temp(ty);
        auto zero = materialize_const(0, ty);
        auto one = materialize_const(1, ty);
        auto rptr = new_temp(ptr_to(ty));

        ops::AddrOf ao;
        ao.dst = Value::obj(rptr);
        ao.src = Value::obj(result);
        emit(std::move(ao));

        ops::Store st0;
        st0.ptr = Value::obj(rptr);
        st0.value = zero;
        emit(std::move(st0));

        unsigned l_true = new_label();
        unsigned l_check = new_label();
        unsigned l_end = new_label();

        auto lhs = lower_expr(child(n, types::BinaryOp::LEFT).value());

        ops::Branch br;
        br.cond = lhs;
        br.true_label = Value::lbl(l_true);
        br.false_label = Value::lbl(l_check);
        emit(std::move(br));

        ops::Label lt;
        lt.label = Value::lbl(l_true);
        emit(std::move(lt));

        ops::Store st1;
        st1.ptr = Value::obj(rptr);
        st1.value = one;
        emit(std::move(st1));

        ops::Jump j1;
        j1.label = Value::lbl(l_end);
        emit(std::move(j1));

        ops::Label lc;
        lc.label = Value::lbl(l_check);
        emit(std::move(lc));

        auto rhs = lower_expr(child(n, types::BinaryOp::RIGHT).value());

        ops::Store st2;
        st2.ptr = Value::obj(rptr);
        st2.value = rhs;
        emit(std::move(st2));

        ops::Label le;
        le.label = Value::lbl(l_end);
        emit(std::move(le));

        return Value::obj(result);
    }

    Value lower_call(node_iter n) {
        auto callee = child(n, types::Call::EXPR).value();
        auto arg_nodes = children_from(n, types::Call::ARG0);

        unsigned fn_id = 0;
        bool direct = false;
        if (callee->is<types::ID>()) {
            auto &id = callee->get<types::ID>();
            if (auto *f = tu.find_function_by_name(id.name)) {
                fn_id = f->id;
                direct = true;
            }
        }

        if (!direct)
            throw std::runtime_error(
                "indirect call is not yet supported in HIR"
            );

        object_type ret_ty = n->type.value();
        unsigned result = new_temp(ret_ty); // даже для void — нужен объект

        ops::Call op;
        op.fn = Value::fn(fn_id);
        op.dst = Value::obj(result);
        for (auto a : arg_nodes) op.varargs.push_back(lower_expr(a));
        emit(std::move(op));

        return Value::obj(result);
    }

    Value lower_sizeof(node_iter n) {
        auto of = child(n, types::SizeOf::OF).value();
        auto of_ty = of->type.value();
        auto sz = of_ty.size_of(tt);
        return materialize_const(sz, n->type.value());
    }

    Value lower_string(node_iter n) {
        auto &lit = n->get<types::StringLiteral>();
        object_type ty = n->type.value();

        // строковая константа
        hir::Constant c;
        c.type = ty;
        c.value = std::string(lit.value); // variant → std::string
        auto cid = tu.add_constant(std::move(c));

        // глобальный объект-хранилище строки
        object_type data_ty = ty;
        if (data_ty.is_pointer(tt)) data_ty = data_ty.pointee_type(tt);

        hir::Object o;
        o.name = "__str" + std::to_string(next_global++);
        o.type = data_ty;
        o.constant_id = cid;
        o.is_global = true;
        o.is_const = true;
        auto oid = tu.add_object(std::move(o));

        auto ptr = new_temp(ty);
        ops::AddrOf ao;
        ao.dst = Value::obj(ptr);
        ao.src = Value::obj(oid);
        emit(std::move(ao));
        return Value::obj(ptr);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// входная точка
// ---------------------------------------------------------------------------

std::unique_ptr<contract::hir::TranslationUnit> Ast::make_hir() {
    auto tu = std::make_unique<contract::hir::TranslationUnit>();
    tu->type_table = &types;
    HirBuilder builder(*tu, types);
    builder.build(*this);
    return tu;
}

} // namespace frontend::ast