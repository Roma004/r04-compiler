#include "frontend/ast/ast.hpp"
#include <fstream>
#include <iostream>

#include <frontend/parser.hpp>

int main(void) {
    using std::to_string;
    std::string input = "";
    std::string rd = "";
    while (std::getline(std::cin, rd)) input += rd + '\n';
    std::fstream fout;

    frontend::parser::ParserContext ctx(input);

    if (!ctx.parse()) std::cerr << "Invalid input!" << std::endl;

    if (!ctx.errors.empty()) {
        for (auto &msg : ctx.errors) {
            std::cerr << std::format(
                "Syntax Error: `{}` at: {}:{}",
                msg.msg,
                msg.loc.line,
                msg.loc.column
            ) << std::endl;
        }
    }

    fout.open("asd.dot", std::ios_base::out);
    ctx.ast.to_dot(
        fout,
        [](const auto &n) -> helpers::DOTList {
            return {{"label", frontend::ast::to_string(n)}};
        },
        [](const auto &e) -> helpers::DOTList {
            return {{"label", frontend::ast::to_string(e)}};
        }
    );
    fout.close();

    return 0;
}
