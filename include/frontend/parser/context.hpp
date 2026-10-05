#pragma once

#include <frontend/ast/ast.hpp>
#include <frontend/parser/source_map.hpp>
#include <string_view>
#include <vector>

#include <ctpg/parse-table.hpp>

namespace frontend::parser {

struct ParserContext {
    std::vector<ErrorMessage> errors;
    SourceMap src_map;
    ast::Ast ast;

    ParserContext(std::string_view input) : src_map(input) {}

    void insert_error(const std::string &msg, std::string_view locate) {
        errors.emplace_back(msg, src_map.locate(locate));
    }

    bool parse();
};

ctpg::parse_table &get_parse_table() noexcept;

} // namespace frontend::parser
