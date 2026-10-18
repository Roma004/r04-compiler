#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

#include <frontend/ast/ast.hpp>
#include <frontend/parser/context.hpp>

namespace {

enum class Stage { Syntax, SyntaxSemantic };

void print_usage(std::string_view prog) {
    std::cout << "usage: " << prog
              << " [--syntax|--syntax-semantic|--syntax-semantic-IR] "
                 "<filename>"
              << std::endl;
}

Stage parse_args(int argc, char **argv, const char *&filename) {
    Stage stage = Stage::SyntaxSemantic;
    filename = nullptr;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--syntax") {
            stage = Stage::Syntax;
        } else if (arg == "--syntax-semantic" || arg == "--semantic") {
            stage = Stage::SyntaxSemantic;
        } else if (arg == "--syntax-semantic-IR" || arg == "--ir") {
            // IR output is not implemented yet; run the deepest available
            // stage so the option is at least accepted.
            // TODO: add the real IR pipeline here.
            stage = Stage::SyntaxSemantic;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            std::exit(2);
        } else if (filename == nullptr) {
            filename = argv[i];
        } else {
            std::cerr << "unexpected argument: " << arg << std::endl;
            print_usage(argv[0]);
            std::exit(2);
        }
    }

    if (filename == nullptr) {
        print_usage(argv[0]);
        std::exit(2);
    }

    return stage;
}

} // namespace

int main(int argc, char **argv) {
    const char *filename = nullptr;
    Stage stage = parse_args(argc, argv, filename);

    std::ifstream fin;
    fin.open(filename);
    if (!fin) {
        std::cerr << "cannot open file: " << filename << std::endl;
        return 1;
    }

    std::string input;
    std::string rd;
    while (std::getline(fin, rd)) input += rd + '\n';

    frontend::parser::ParserContext ctx(input);

    if (!ctx.parse()) {
        std::cerr << "Invalid input!" << std::endl;
        return 1;
    }

    if (stage == Stage::SyntaxSemantic) {
        if (!ctx.ast.check_semantics(ctx)) {
            for (auto &msg : ctx.errors) {
                std::cerr << std::format(
                    "Syntax Error: `{}` at: {}:{}",
                    msg.msg,
                    msg.loc.line,
                    msg.loc.column
                ) << std::endl;
            }
            return 1;
        }
    }

    std::fstream fout;
    fout.open("asd.dot", std::ios_base::out);
    ctx.ast.to_dot(
        fout,
        /* node attrs */ {{"shape", "record"}},
        /* node conv  */
        [&ctx](const auto &n) -> helpers::DOTList {
            return {
                {"label", ctx.ast.repr_node(n)},
            };
        },
        /* edge conv  */
        [&ctx](const auto &e) -> helpers::DOTList {
            return {{"label", ctx.ast.repr_edge(e)}};
        }
    );
    fout.close();

    return 0;
}
