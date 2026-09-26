#include "frontend/ast/ast.hpp"
#include <iostream>
#include <fstream>

#include <frontend/parser.hpp>


int main(void) {
    using std::to_string;
    std::string input;
    std::getline(std::cin, input);
    std::fstream fout;

    frontend::parser::ParserContext ctx(input);
    ctx.parse();

    fout.open("asd.dot", std::ios_base::out);
    ctx.ast.to_dot(
        fout,
        [](const frontend::ast::node_t &n) -> helpers::DOTList {
            return {
                {"label", to_string(n)},
            };
        },
        [](int n) -> helpers::DOTList { return {{"label", std::to_string(n)}}; }
    );
    fout.close();

    return 0;
}
