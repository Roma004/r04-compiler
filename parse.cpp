#include <fstream>
#include <iostream>

#include <frontend/ast/ast.hpp>
#include <frontend/parser/context.hpp>

int main(int argc, char **argv) {
    using std::to_string;

    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::ifstream fin;
    fin.open(argv[1]);

    std::string input = "";
    std::string rd = "";
    while (std::getline(fin, rd)) input += rd + '\n';
    std::fstream fout;

    frontend::parser::ParserContext ctx(input);

    if (!ctx.parse()) std::cerr << "Invalid input!" << std::endl;

    if (!ctx.ast.check_semantics(ctx)) {
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
