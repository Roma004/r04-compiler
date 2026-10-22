#include "frontend/ast/type_info.hpp"

#include <cassert>
#include <utility>

namespace frontend::ast {

void node_type::insert_pointer(bool is_const) {
    mods.push_back({.kind = modifier::Kind::Pointer, .is_const = is_const});
}

void node_type::insert_const() {
    if (mods.empty()) is_const = true;
    else mods.back().is_const = true;
}

void node_type::insert_array(size_t count, bool is_const) {
    mods.push_back(
        {.kind = modifier::Kind::Array, .count = count, .is_const = is_const}
    );
}

bool node_type::is_function(const TypeTable &tb) const noexcept {
    return tb.get(type_id).is_function;
}

node_type node_type::return_type(const TypeTable &tb) const {
    assert(is_function(tb));
    return tb.get(type_id).return_type;
}

size_t node_type::size_of(const TypeTable &tb) const noexcept {
    size_t sz = tb.get(type_id).size;
    for (const auto &m : mods) {
        switch (m.kind) {
        case modifier::Kind::Pointer: sz = sizeof(void *); break;
        case modifier::Kind::Array: sz *= m.count; break;
        }
    }
    return sz;
}

size_t node_type::align_of(const TypeTable &tb) const noexcept {
    size_t a = tb.get(type_id).align;
    for (const auto &m : mods) {
        switch (m.kind) {
        case modifier::Kind::Pointer: a = alignof(void *); break;
        case modifier::Kind::Array: break;
        }
    }
    return a;
}

bool type_info::add_field(
    const std::string &fname, const node_type &ftype, const TypeTable &tb
) {
    for (const auto &f : fields)
        if (f.name == fname) return false;

    size_t f_size = ftype.size_of(tb);
    size_t f_align = ftype.align_of(tb);

    size_t offset = size;
    if (!is_packed && f_align > 1) {
        size_t rem = offset % f_align;
        if (rem) offset += f_align - rem;
    }

    fields.push_back({
        .type = ftype,
        .name = fname,
        .offset = offset,
        .size = f_size,
    });

    size = offset + f_size;
    if (!is_packed && f_align > align) align = f_align;
    return true;
}

void type_info::update_size() {
    if (!is_packed && align > 1) {
        size_t rem = size % align;
        if (rem) size += align - rem;
    }
    if (size == 0) size = 1;
}

std::string node_type::repr(const TypeTable &tb) const {
    const type_info &ti = tb.get(type_id);

    if (ti.is_function) {
        std::string res = ti.return_type.repr(tb) + " (";
        for (size_t i = 0; i < ti.fields.size(); ++i) {
            if (i) res += ", ";
            res += ti.fields[i].type.repr(tb);
        }
        res += ")";
        return res;
    }

    std::string res(ti.name);

    if (is_const) res += " const";

    for (auto mod : mods) {
        switch (mod.kind) {
        case modifier::Kind::Pointer: res += " *"; break;
        case modifier::Kind::Array:
            res += " [" + std::to_string(mod.count) + "]";
            break;
        }
        if (mod.is_const) res += " const";
    }

    return res;
}

TypeTable::TypeTable() noexcept {
    insert_basic_type("uint64_t", 8, false);
    insert_basic_type("uint32_t", 4, false);
    insert_basic_type("uint16_t", 2, false);
    insert_basic_type("uint8_t", 1, false);
    insert_basic_type("int64_t", 8, true);
    insert_basic_type("int32_t", 4, true);
    insert_basic_type("int16_t", 2, true);
    insert_basic_type("int8_t", 1, true);
    insert_basic_type("bool_t", 1, false);
    insert_basic_type("void_t", 1, false);
}

unsigned TypeTable::insert_type(type_info &&type) {
    unsigned idx = types.size();

    // first emplace name into mapping, type stores reference into it
    auto it = mapping.emplace(type.name, idx);
    const std::string &name = it.first->first;

    // then move the type and update it's fields
    types.push_back(std::move(type));
    types.back().type_id = idx;
    types.back().name = name;

    return idx;
}

bool TypeTable::has_type(std::string_view sv) const {
    return mapping.count(sv);
}

unsigned TypeTable::get_id(std::string_view sv) const {
    auto it = mapping.find(sv);
    if (it == mapping.end()) {
        throw std::runtime_error("type not found: " + std::string(sv));
    }
    return it->second;
}

type_info &TypeTable::get(std::string_view sv) { return types[get_id(sv)]; }

const type_info &TypeTable::get(std::string_view sv) const {
    return types[get_id(sv)];
}

type_info &TypeTable::get(unsigned idx) { return types[idx]; }

const type_info &TypeTable::get(unsigned idx) const { return types[idx]; }

void TypeTable::insert_basic_type(
    std::string_view name, size_t size, bool is_signed
) {
    insert_type(
        {.name = name,
         .size = size,
         .align = (uint8_t)size,
         .is_signed = is_signed,
         .is_integral = name != "void_t"}
    );
}

}; // namespace frontend::ast
