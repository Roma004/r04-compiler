#include "frontend/ast/types.hpp"
#include <cassert>

namespace frontend::ast::types {

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

inline uint8_t decode_escape(std::string_view &sv) {
    if (sv.empty()) return 0;
    char c = sv[0];
    sv.remove_prefix(1);
    switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'v': return '\v';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    case '?': return '?';
    case '0': return '\0';
    case 'x': {
        unsigned v = 0;
        while (!sv.empty() && hex_digit(sv[0]) >= 0) {
            v = v * 16u + hex_digit(sv[0]);
            sv.remove_prefix(1);
        }
        return v;
    }
    default:
        if (c >= '0' && c <= '7') {
            unsigned v = c - '0';
            for (int i = 0;
                 i < 2 && !sv.empty() && sv[0] >= '0' && sv[0] <= '7';
                 ++i) {
                v = v * 8u + sv[0] - '0';
                sv.remove_prefix(1);
            }
            return v;
        }
        return c;
    }
}

CharLiteral::CharLiteral(std::string_view sv) {
    if (sv.size() >= 2 && sv.front() == '\'' && sv.back() == '\'')
        sv = sv.substr(1, sv.size() - 2);
    if (sv.empty()) return;
    if (sv[0] == '\\') {
        sv.remove_prefix(1);
        value = decode_escape(sv);
    } else {
        value = sv[0];
    }
};

StringLiteral::StringLiteral(std::string_view sv) {
    if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"')
        sv = sv.substr(1, sv.size() - 2);

    value.reserve(sv.size());
    while (!sv.empty()) {
        char c = sv[0];
        sv.remove_prefix(1);
        if (c == '\\') {
            value.push_back(decode_escape(sv));
        } else {
            value.push_back(c);
        }
    }
}

Literal::Literal(std::string_view sv) {
    std::string_view digits = sv;
    int base = 10;
    if (digits.size() > 2 && digits[0] == '0'
        && (digits[1] == 'x' || digits[1] == 'X')) {
        base = 16;
        digits.remove_prefix(2);
    } else if (
        digits.size() > 2 && digits[0] == '0'
        && (digits[1] == 'b' || digits[1] == 'B')
    ) {
        base = 2;
        digits.remove_prefix(2);
    } else if (digits.size() > 1 && digits[0] == '0') {
        base = 8;
        digits.remove_prefix(1);
    }

    size_t suffix_start = digits.find_first_not_of("0123456789abcdefABCDEF");
    std::string_view suffix;
    if (suffix_start != std::string_view::npos) {
        suffix = digits.substr(suffix_start);
        digits = digits.substr(0, suffix_start);
    }

    uint64_t val = 0;
    auto [ptr, ec] = std::from_chars(
        digits.data(), digits.data() + digits.size(), val, base
    );
    if (ec != std::errc{}) throw std::runtime_error("invalid integer literal");

    bool has_u = false, has_l = false, has_ll = false;
    for (char c : suffix) {
        if (c == 'u' || c == 'U') has_u = true;
        else if (c == 'l' || c == 'L') {
            if (has_l) has_ll = true;
            else has_l = true;
        }
    }

    size = has_l ? 8 : 4;
    is_signed = !has_u;

    value = val & mask_for_size(size);
}

uint64_t Literal::mask_for_size(uint8_t sz) {
    if (sz >= 8) return ~0ULL;
    return (1ULL << (sz * 8)) - 1ULL;
}

int64_t Literal::sign_extend(uint64_t v, uint8_t sz) {
    if (sz >= 8) return v;
    uint64_t m = mask_for_size(sz);
    v &= m;
    uint64_t sign_bit = 1ULL << (sz * 8 - 1);
    return v & sign_bit ? v | ~m : v;
}

uint64_t Literal::convert_to(
    uint64_t val,
    uint8_t from_size,
    bool from_signed,
    uint8_t to_size,
    bool to_signed
) {
    val &= mask_for_size(from_size);
    if (from_size < to_size && from_signed) {
        val = sign_extend(val, from_size);
    }
    return val & mask_for_size(to_size);
}

#define APPLY(_op_, type)                                        \
    if (op == #_op_) {                                           \
        if (common_signed) return make_##type(lhs_s _op_ rhs_s); \
        else return make_##type(lhs _op_ rhs);                   \
    }

Literal Literal::apply(std::string_view op, const Literal &other) const {
    uint8_t common_size = std::max(size, other.size);
    bool common_signed;
    if (size == other.size) {
        common_signed = is_signed && other.is_signed;
    } else if (size > other.size) {
        common_signed = is_signed;
    } else {
        common_signed = other.is_signed;
    }

    uint64_t lhs =
        convert_to(value, size, is_signed, common_size, common_signed);
    uint64_t rhs = convert_to(
        other.value, other.size, other.is_signed, common_size, common_signed
    );

    int64_t lhs_s = sign_extend(lhs, common_size);
    int64_t rhs_s = sign_extend(rhs, common_size);

    auto make_int = [&](uint64_t val) {
        return Literal(
            val & mask_for_size(common_size), common_size, common_signed
        );
    };
    auto make_bool = [](bool b) { return Literal(b ? 1ULL : 0ULL, 4, true); };

    // clang-format off
         APPLY(||, bool) else APPLY(&&, bool) else APPLY(==, bool)
    else APPLY(!=, bool) else APPLY(<=, bool) else APPLY(>=, bool)
    else APPLY(<,  bool) else APPLY(>,  bool) else APPLY(|,  int)
    else APPLY(^,  int)  else APPLY(&,  int)  else APPLY(<<, int)
    else APPLY(>>, int)  else APPLY(+,  int)  else APPLY(-,  int)
    else APPLY(*,  int)  else APPLY(/,  int)  else APPLY(%,  int)
    assert("Got invalid operation" == 0);
    // clang-format on
}

#undef APPLY
#define APPLY(_op_, type)                              \
    if (op == #_op_) {                                 \
        if (is_signed) return make_##type(_op_ rhs_s); \
        else return make_##type(_op_ rhs);             \
    }

Literal Literal::apply(std::string_view op) const {
    uint64_t rhs = value;
    int64_t rhs_s = sign_extend(rhs, size);

    auto make_int = [&](uint64_t val) {
        return Literal(val & mask_for_size(size), size, is_signed);
    };
    auto make_bool = [](bool b) { return Literal(b ? 1ULL : 0ULL, 4, true); };

    // clang-format off
    APPLY(-, int) else APPLY(~, int) else APPLY(!, bool)
    assert("Got invalid operation" == 0);
    // clang-format on
}
#undef APPLY

TypeSpec::TypeSpec(std::string_view sv) : name(sv), attrs(0) {
    if (sv == "void_t") {
        attrs |= INTEGRAL;
        size = 1;
    } else if (sv == "bool_t") {
        attrs |= INTEGRAL;
        size = 1;
    } else if (sv == "uint8_t") {
        attrs |= INTEGRAL;
        size = 1;
    } else if (sv == "uint16_t") {
        attrs |= INTEGRAL;
        size = 2;
    } else if (sv == "uint32_t") {
        attrs |= INTEGRAL;
        size = 4;
    } else if (sv == "uint64_t") {
        attrs |= INTEGRAL;
        size = 8;
    } else if (sv == "int8_t") {
        attrs |= INTEGRAL | SIGNED;
        size = 1;
    } else if (sv == "int16_t") {
        attrs |= INTEGRAL | SIGNED;
        size = 2;
    } else if (sv == "int32_t") {
        attrs |= INTEGRAL | SIGNED;
        size = 4;
    } else if (sv == "int64_t") {
        attrs |= INTEGRAL | SIGNED;
        size = 8;
    }
}
}; // namespace frontend::ast::types
