#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "contract/hir/operations.hpp"
#include "contract/hir/type_info.hpp"

namespace contract::hir {

struct Object {
    unsigned id = 0;
    std::string name;
    object_type type;
    unsigned constant_id;
    bool is_global = false;
    bool is_automatic = false;
    bool is_const = false;
};

struct Constant {
    unsigned id = 0;
    object_type type;
    std::variant<uint64_t, std::string> value;
};

// clang-format off
using OpVariant = std::variant<
    ops::Add, ops::Sub, ops::Mul, ops::Div, ops::Mod,
    ops::And, ops::Or, ops::Xor, ops::Shl, ops::Shr,

    ops::Neg, ops::Not, ops::Inv,

    ops::Eq, ops::Ne, ops::Lt, ops::Le, ops::Gt, ops::Ge,

    ops::Const,
    ops::AddrOf,
    ops::Load,
    ops::Store,
    ops::ElemPtr,
    ops::FieldPtr,
    ops::Cast,
    ops::Label,
    ops::Jump,
    ops::Branch,
    ops::Call,
    ops::Return>;
// clang-format on

struct Operation {
    OpVariant op;

    void check(std::string_view top) {
        std::visit(
            [&](auto &o) {
                o.check_args(top);
                if constexpr (requires { o.check_varargs(top); })
                    o.check_varargs(top);
            },
            op
        );
    }
};

struct Function {
    unsigned id = 0;
    std::string name;
    object_type return_type;
    std::vector<Value> params;
    std::vector<Value> locals;
    std::vector<Operation> ops;
    Value address_obj;
    bool is_external = false;
};

class TranslationUnit {
  public:
    std::vector<Object> objects;
    std::vector<Constant> constants;
    std::vector<Function> functions;
    TypeTable *type_table = nullptr;

    Object &object(Value id) {
        id.check_is(Value::Kind::object, "get_object: ");
        return objects[id.id];
    }
    Constant &constant(Value id) {
        id.check_is(Value::Kind::constant, "get_constant: ");
        return constants[id.id];
    }
    Function &function(Value id) {
        id.check_is(Value::Kind::function, "get_function: ");
        return functions[id.id];
    }

    Value add_object(Object o) {
        o.id = objects.size();
        objects.push_back(std::move(o));
        return Value::obj(o.id);
    }
    Value add_constant(Constant c) {
        c.id = constants.size();
        constants.push_back(std::move(c));
        return Value::cst(c.id);
    }
    Value add_function(Function f) {
        f.id = functions.size();
        functions.push_back(std::move(f));
        return Value::fn(f.id);
    }

    Function *find_function_by_name(std::string_view name) {
        for (auto &f : functions)
            if (f.name == name) return &f;
        return nullptr;
    }
    Object *find_global_by_name(std::string_view name) {
        for (auto &o : objects)
            if (o.is_global && o.name == name) return &o;
        return nullptr;
    }
};

}; // namespace contract::hir