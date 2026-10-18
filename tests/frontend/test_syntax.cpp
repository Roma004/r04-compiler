#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

namespace {

struct run_result {
    int exit_code = -1;
    std::string stdout_text;
    std::string stderr_text;
    std::string dot_text;
    bool dot_exists = false;
};

std::string read_text(const std::filesystem::path &path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string quote(const std::string &value) {
    return "\"" + value + "\"";
}

// Strip libgcov noise produced by the coverage-instrumented parser.
std::string filtered_stderr(const std::string &text) {
    std::istringstream in(text);
    std::string line;
    std::string out;
    while (std::getline(in, line)) {
        if (line.rfind("libgcov", 0) == 0) continue;
        out += line + '\n';
    }
    return out;
}

run_result run_parser(
    const std::string &stage, const std::filesystem::path &input
) {
    static std::atomic<int> counter{0};
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path()
        / ("r04_syntax_" + std::to_string(counter++));
    std::filesystem::create_directories(dir);

    // GCOV_PREFIX redirects gcda files away from the build tree, so the
    // coverage-instrumented parser does not emit libgcov warnings.
    const std::string cmd = "cd " + quote(dir.string()) + " && GCOV_PREFIX="
        + quote((dir / "gcov").string()) + " " + quote(PARSER_BINARY_PATH)
        + " --" + stage + " " + quote(input.string())
        + " >stdout.txt 2>stderr.txt; echo $? >status.txt";
    std::system(cmd.c_str());

    run_result res;
    res.stdout_text = read_text(dir / "stdout.txt");
    res.stderr_text = read_text(dir / "stderr.txt");
    res.exit_code = std::stoi(read_text(dir / "status.txt"));
    res.dot_exists = std::filesystem::exists(dir / "asd.dot");
    if (res.dot_exists) res.dot_text = read_text(dir / "asd.dot");

    std::filesystem::remove_all(dir);
    return res;
}

std::filesystem::path inputs_dir() {
    return std::filesystem::path(TEST_SOURCE_DIR) / "tests" / "frontend"
        / "inputs";
}

} // namespace

TEST_CASE("syntax stage accepts every valid example", "[syntax]") {
    const std::filesystem::path ok_dir = inputs_dir() / "ok";

    for (const auto &entry : std::filesystem::directory_iterator(ok_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        const run_result res = run_parser("syntax", entry.path());
        REQUIRE(res.exit_code == 0);
        REQUIRE(filtered_stderr(res.stderr_text).empty());
        REQUIRE(res.dot_exists);
        REQUIRE(res.dot_text.rfind("digraph {", 0) == 0);
    }
}

TEST_CASE("syntax-semantic stage accepts every valid example", "[syntax]") {
    const std::filesystem::path ok_dir = inputs_dir() / "ok";

    for (const auto &entry : std::filesystem::directory_iterator(ok_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        const run_result res = run_parser("syntax-semantic", entry.path());
        REQUIRE(res.exit_code == 0);
        REQUIRE(filtered_stderr(res.stderr_text).empty());
        REQUIRE(res.dot_exists);
        REQUIRE(res.dot_text.rfind("digraph {", 0) == 0);
    }
}

TEST_CASE("syntax stage rejects invalid syntax", "[syntax]") {
    const std::filesystem::path bad_dir = inputs_dir() / "bad" / "syntax";

    for (const auto &entry : std::filesystem::directory_iterator(bad_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        const run_result res = run_parser("syntax", entry.path());
        REQUIRE(res.exit_code != 0);
        REQUIRE_FALSE(filtered_stderr(res.stderr_text).empty());
        REQUIRE_FALSE(res.dot_exists);
    }
}

TEST_CASE("syntax-semantic stage rejects semantically invalid examples", "[syntax]") {
    const std::filesystem::path bad_dir = inputs_dir() / "bad" / "semantic";

    for (const auto &entry : std::filesystem::directory_iterator(bad_dir)) {
        if (!entry.is_regular_file()) continue;
        INFO("file: " << entry.path().filename().string());

        // These files are syntactically valid, so the syntax stage passes.
        const run_result syntax_res = run_parser("syntax", entry.path());
        REQUIRE(syntax_res.exit_code == 0);
        REQUIRE(syntax_res.dot_exists);

        const run_result semantic_res =
            run_parser("syntax-semantic", entry.path());
        REQUIRE(semantic_res.exit_code != 0);
        REQUIRE(filtered_stderr(semantic_res.stderr_text).find("Syntax Error:")
                != std::string::npos);
        REQUIRE_FALSE(semantic_res.dot_exists);
    }
}

TEST_CASE("IR stage option is accepted", "[syntax]") {
    const std::filesystem::path input = inputs_dir() / "ok" / "control_flow.r04";
    const run_result res = run_parser("syntax-semantic-IR", input);
    REQUIRE(res.exit_code == 0);
    REQUIRE(filtered_stderr(res.stderr_text).empty());
    REQUIRE(res.dot_exists);
}
