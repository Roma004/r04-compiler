#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "tools/graph.hpp"
#include "tools/macro_template.hpp"

namespace {

using graph_t = tools::orgraph_t<int, std::string>;
using tools::graph::InspectDirection;
using tools::graph::InspectType;

using edge_iter = graph_t::edge_iter;
using node_iter = graph_t::node_iter;

// Tag types for the template test case — one per combination of
// traversal type and direction
struct dfs_fwd {
    static constexpr InspectType type = InspectType::DFS;
    static constexpr InspectDirection dir = InspectDirection::FORWARD;
};
struct dfs_bwd {
    static constexpr InspectType type = InspectType::DFS;
    static constexpr InspectDirection dir = InspectDirection::BACKWARD;
};
struct bfs_fwd {
    static constexpr InspectType type = InspectType::BFS;
    static constexpr InspectDirection dir = InspectDirection::FORWARD;
};
struct bfs_bwd {
    static constexpr InspectType type = InspectType::BFS;
    static constexpr InspectDirection dir = InspectDirection::BACKWARD;
};

// Builds the diamond
//
//       1
//      / \
//     2   3
//      \ /
//       4
//
// - node 1 reaches every other node along forward edges
// - node 4 reaches every other node along backward edges
std::array<node_iter, 4> build_diamond(graph_t &g) {
    node_iter a = g.emplace_node(1);
    node_iter b = g.emplace_node(2);
    node_iter c = g.emplace_node(3);
    node_iter d = g.emplace_node(4);
    g.emplace_edge(a, b, "ab");
    g.emplace_edge(a, c, "ac");
    g.emplace_edge(b, d, "bd");
    g.emplace_edge(c, d, "cd");
    return {a, b, c, d};
}

} // namespace

#define INSPECT_ARGS int &n1, std::string &e, int &n2, bool is_visited
#define INSPECT_ITER_ARGS \
    node_iter n1, edge_iter e, node_iter n2, bool is_visited

TEMPLATE_TEST_CASE(
    "graph traverse inspect",
    "[graph][traverse]",
    dfs_fwd,
    dfs_bwd,
    bfs_fwd,
    bfs_bwd
) {
    constexpr auto type = TestType::type;
    constexpr auto dir = TestType::dir;

    SECTION("traversal visits every node reachable from the start") {
        graph_t g;
        auto &&[a, b, c, d] = build_diamond(g);

        // Start from the node that sees all others in the given direction
        node_iter start = (dir == InspectDirection::FORWARD) ? a : d;

        // Every node except the start is expected to be discovered
        std::vector<int> expected;
        for (auto it = g.begin(); it != g.end(); ++it) {
            if (it == start) continue;
            expected.push_back(it->get_data());
        }

        std::vector<int> discovered;
        g.template traverse<type, dir>(start, [&](INSPECT_ARGS) {
            if (!is_visited) discovered.push_back(n2);
            return true;
        });

        std::sort(expected.begin(), expected.end());
        std::sort(discovered.begin(), discovered.end());

        REQUIRE(discovered == expected);
    }

    SECTION("backward traversal from any node reaches the start") {
        if constexpr (dir == InspectDirection::BACKWARD) {
            graph_t g;
            auto &&[a, b, c, d] = build_diamond(g);

            // From any node except the start going backward the traversal
            // must eventually reach the start node 1
            for (auto start : {b, c, d}) {
                std::vector<int> discovered;
                g.template traverse<type, dir>(start, [&](INSPECT_ARGS) {
                    if (!is_visited) discovered.push_back(n2);
                    return true;
                });
                REQUIRE(
                    std::count(discovered.begin(), discovered.end(), 1) == 1
                );
            }
        }
    }

    SECTION("unreachable nodes are not reported") {
        graph_t g;
        auto &&[a, b, c, d] = build_diamond(g);
        g.emplace_node(123);

        node_iter start = (dir == InspectDirection::FORWARD) ? a : d;

        std::vector<int> discovered;
        g.template traverse<type, dir>(start, [&](INSPECT_ARGS) {
            if (!is_visited) discovered.push_back(n2);
            return true;
        });

        REQUIRE(std::count(discovered.begin(), discovered.end(), 99) == 0);
        REQUIRE(std::count(discovered.begin(), discovered.end(), 100) == 0);
    }

    SECTION("single node produces no handle calls") {
        graph_t g;
        auto a = g.emplace_node(123);

        int calls = 0;
        g.template traverse<type, dir>(a, [&](INSPECT_ARGS) {
            if (!is_visited) ++calls;
            return true;
        });

        REQUIRE(calls == 0);
    }

    SECTION("each traversed edge is reported at most once") {
        graph_t g;
        auto &&[a, b, c, d] = build_diamond(g);

        // Start from the node that sees all others in the given direction
        node_iter start = (dir == InspectDirection::FORWARD) ? a : d;

        std::vector<edge_iter> reported;
        g.template traverse<type, dir>(start, [&](INSPECT_ITER_ARGS) {
            reported.push_back(e);
            return true;
        });

        // No edge may appear twice in the reported sequence
        std::set<std::string> seen;
        for (auto e : reported) { REQUIRE(seen.insert(e->get_data()).second); }

        REQUIRE(reported.size() == 4);
    }
}
