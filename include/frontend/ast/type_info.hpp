#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "tools/macro_template.hpp"

namespace frontend::ast {

class TypeTable;

/**
 * type of node with const, pointer and array extensions
 *
 * Modifiers are stored from the innermost one to the outermost one, so
 * `uint32_t *[5][3]` is an array of three arrays of five pointers to
 * `uint32_t`. `const` is stored on the level of the object it qualifies:
 * `is_const` for the base type, `modifier::is_const` for pointers and arrays.
 */
struct node_type {
    struct modifier {
        enum class Kind : uint8_t { Pointer, Array };
        Kind kind = Kind::Pointer;
        uint64_t count = 0;
        bool is_const = false;
    };

    std::vector<modifier> mods;
    unsigned type_id = 0;
    bool is_const = false;

    /**
     * @brief put a pointer on top of the current type
     *
     * @param is_const   wether pointer itself might be assigned
     */
    void insert_pointer(bool is_const);

    /** put an array on top of the current type */
    void insert_array(size_t count, bool is_const);

    bool is_function(const TypeTable &tb) const noexcept;
    node_type return_type(const TypeTable &tb) const;

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

    // function types: return type and parameters (parameters are stored in
    // `fields` using the same engine as structure fields)
    node_type return_type;
    bool is_function = false;

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