#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <tools/macro_template.hpp>

namespace frontend::ast {

class TypeTable;

/** type of node with const, pointer and array extensions */
struct node_type {
    struct modifier {
        enum class Kind : uint8_t { Const, Pointer, Array };
        Kind kind = Kind::Const;
        uint64_t count = 0;
    };

    std::vector<modifier> mods;
    unsigned type_id = 0;

    void insert_pointer();
    void insert_const();
    void insert_array(size_t count);

    size_t size_of(const TypeTable &tb) const noexcept;
    size_t align_of(const TypeTable &tb) const noexcept;

    std::string repr(const TypeTable &tb) const;
};

/** basic in-language type */
struct type_info {
    struct field {
        node_type type;
        std::string name;
        size_t offset = 0;
        size_t size = 0;
    };

    std::string_view name;
    std::vector<field> fields = {};
    size_t size = 0;
    unsigned type_id = 0;
    uint8_t align = 1;
    bool is_signed = false;
    bool is_integral = true;
    bool is_packed = false;

    bool add_field(
        const std::string &name, const node_type &type, const TypeTable &tb
    );

    void update_size();
};

class TypeTable {
  public:
    TypeTable() noexcept;

    unsigned insert_type(type_info &&type);

    bool has_type(std::string_view sv) const;

    unsigned get_id(std::string_view sv) const;

    type_info &get(std::string_view sv);
    const type_info &get(std::string_view sv) const;
    type_info &get(unsigned idx);
    const type_info &get(unsigned idx) const;

  private:
    std::vector<type_info> types;
    helpers::string_map<unsigned> mapping;

    void insert_basic_type(std::string_view name, size_t size, bool is_signed);
};

}; // namespace frontend::ast