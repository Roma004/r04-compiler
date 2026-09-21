#include <algorithm>
#include <catch2/catch_all.hpp>

#include <catch2/catch_test_macros.hpp>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "tools/graph.hpp"

namespace {

using tools::graph::InspectDirection;
using tools::graph::InspectType;

using graph_t = tools::orgraph_t<int, std::string>;
using node_iter = graph_t::node_iter;
using edge_iter = graph_t::edge_iter;

// Traverse from start with the default DFS/FORWARD and collect the data of
// every edge that was used to discover a new node = InspectDirection::FORWARD
template <
    InspectType type = InspectType::DFS,
    InspectDirection dir = InspectDirection::FORWARD>
std::vector<std::string> traverse(graph_t &g, graph_t::node_iter start) {
    std::vector<std::string> result;
    g.template traverse<type, dir>(
        start,
        [&](const int &, const std::string &e, const int &, bool visited) {
            if (!visited) result.push_back(e);
            return true;
        }
    );
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

TEST_CASE("graph node insertion", "[graph]") {
    graph_t g;
    auto n1 = g.emplace_node(10);
    auto n2 = g.emplace_node(20);
    REQUIRE(n1->get_data() == 10);
    REQUIRE(n2->get_data() == 20);

    // Without edges nothing is traversed
    REQUIRE(traverse(g, n1).empty());
    REQUIRE(traverse(g, n2).empty());

    g.assert_integrity();
}

TEST_CASE("graph with empty types works", "[graph]") {
    tools::orgraph_t<tools::empty, tools::empty> g;
    auto a = g.emplace_node();
    auto b = g.emplace_node();
    auto e = g.emplace_edge(a, b);

    REQUIRE(e->source() == a);
    REQUIRE(e->target() == b);

    STATIC_REQUIRE(std::is_same_v<decltype(a->get_data()), tools::empty &>);
    STATIC_REQUIRE(std::is_same_v<decltype(b->get_data()), tools::empty &>);
    STATIC_REQUIRE(std::is_same_v<decltype(e->get_data()), tools::empty &>);

    g.assert_integrity();
}

TEST_CASE("graph edge insertion", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto e = g.emplace_edge(a, b, "ab");

    REQUIRE(e->source() == a);
    REQUIRE(e->target() == b);
    REQUIRE(e->get_data() == "ab");

    // Inspection from a discovers b along ab
    REQUIRE(traverse(g, a) == std::vector<std::string>{"ab"});
    // Inspection from b does not discover a because ab is a backward edge
    REQUIRE(traverse(g, b).empty());

    g.assert_integrity();
}

TEST_CASE("graph duplicate edge raises", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto e1 = g.emplace_edge(a, b, "first");

    REQUIRE_THROWS_AS(g.emplace_edge(a, b, "second"), std::logic_error);
    REQUIRE(traverse(g, a) == std::vector<std::string>{"first"});

    g.assert_integrity();
}

TEST_CASE("graph opposite edges coexist", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto eab = g.emplace_edge(a, b, "ab");
    auto eba = g.emplace_edge(b, a, "ba");
    REQUIRE(eab != eba);

    REQUIRE(traverse(g, a) == std::vector<std::string>{"ab"});
    REQUIRE(traverse(g, b) == std::vector<std::string>{"ba"});

    g.assert_integrity();
}

TEST_CASE("graph edge removal", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto e = g.emplace_edge(a, b, "ab");

    REQUIRE(traverse(g, a) == std::vector<std::string>{"ab"});

    g.remove_edge(e);

    REQUIRE(traverse(g, a).empty());
    REQUIRE(traverse(g, b).empty());

    g.assert_integrity();
}

TEST_CASE("graph node removal removes incident edges", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    g.emplace_edge(a, b, "ab");
    g.emplace_edge(a, c, "ac");
    g.emplace_edge(b, c, "bc");

    g.remove_node(a);

    // From b we can still reach c along bc, but ab and ac are gone
    REQUIRE(traverse(g, b) == std::vector<std::string>{"bc"});

    // a is no longer exists in graph
    std::vector<int> remaining;
    for (auto &n : g) remaining.push_back(n.get_data());
    REQUIRE(remaining == std::vector<int>{2, 3});

    g.assert_integrity();
}

TEST_CASE("graph edge retargeting", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    auto e = g.emplace_edge(a, b, "ab");

    g.set_egde_target(e, c);

    REQUIRE(e->source() == a);
    REQUIRE(e->target() == c);

    // Traversal from a still uses the same edge, now pointing at c
    REQUIRE(traverse(g, a) == std::vector<std::string>{"ab"});

    g.assert_integrity();
}

TEST_CASE("graph edge resourcing", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    auto e = g.emplace_edge(a, b, "ab");

    g.set_egde_source(e, c);

    REQUIRE(e->source() == c);
    REQUIRE(e->target() == b);

    REQUIRE(traverse(g, c) == std::vector<std::string>{"ab"});
    REQUIRE(traverse(g, a).empty());

    g.assert_integrity();
}

TEST_CASE("graph retarget that duplicates an edge raises", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    auto e_ab = g.emplace_edge(a, b, "ab");
    auto e_ac = g.emplace_edge(a, c, "ac");

    REQUIRE_THROWS_AS(g.set_egde_target(e_ab, c), std::logic_error);

    g.assert_integrity();
}

TEST_CASE("graph resourcing that duplicates an edge raises", "[graph]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    auto e_ab = g.emplace_edge(a, b, "ab");
    auto e_cb = g.emplace_edge(c, b, "cb");

    REQUIRE_THROWS_AS(g.set_egde_source(e_ab, c), std::logic_error);

    g.assert_integrity();
}

TEST_CASE("graph iterators survive mass insertions and removals", "[graph]") {
    struct tracked_node {
        node_iter it;
        int value;
    };

    struct tracked_edge {
        edge_iter it;
        std::string value;
    };

    // Fixed seed keeps the test reproducible
    std::mt19937 rng(12345);

    constexpr size_t total_nodes = 2000;
    constexpr size_t total_edges = 10000;
    constexpr size_t extra_nodes = 1000;
    constexpr size_t extra_edges = 5000;

    graph_t g;

    // insert tracked nodes and remove a random half
    std::vector<tracked_node> nodes;
    nodes.reserve(total_nodes);
    for (size_t i = 0; i < total_nodes; ++i) {
        int v = static_cast<int>(i);
        nodes.push_back({g.emplace_node(v), v});
    }

    g.assert_integrity();
    for (auto &n : nodes) REQUIRE(n.it->get_data() == n.value);

    std::shuffle(nodes.begin(), nodes.end(), rng);
    for (size_t i = 0; i < total_nodes / 2; ++i) { g.remove_node(nodes[i].it); }
    nodes.erase(nodes.begin(), nodes.begin() + total_nodes / 2);

    g.assert_integrity();
    for (auto &n : nodes) REQUIRE(n.it->get_data() == n.value);

    // insert tracked edges and remove a random half
    std::set<std::pair<int, int>> used_pairs;
    std::vector<tracked_edge> edges;
    edges.reserve(total_edges);
    std::uniform_int_distribution<size_t> node_dist(0, nodes.size() - 1);
    while (edges.size() < total_edges) {
        size_t si = node_dist(rng);
        size_t ti = node_dist(rng);
        if (si == ti) continue;
        int sv = nodes[si].value;
        int tv = nodes[ti].value;
        if (!used_pairs.insert({sv, tv}).second) continue;
        std::string data = "e" + std::to_string(edges.size());
        edge_iter e = g.emplace_edge(nodes[si].it, nodes[ti].it, data);
        edges.push_back({e, std::move(data)});
    }

    g.assert_integrity();
    for (auto &e : edges) REQUIRE(e.it->get_data() == e.value);

    std::shuffle(edges.begin(), edges.end(), rng);
    for (size_t i = 0; i < total_edges / 2; ++i) { g.remove_edge(edges[i].it); }
    edges.erase(edges.begin(), edges.begin() + total_edges / 2);

    g.assert_integrity();
    for (auto &n : nodes) REQUIRE(n.it->get_data() == n.value);
    for (auto &e : edges) REQUIRE(e.it->get_data() == e.value);

    // insert untracked nodes and edges around the tracked ones
    std::vector<node_iter> pool;
    pool.reserve(nodes.size() + extra_nodes);
    for (auto &n : nodes) pool.push_back(n.it);
    for (size_t i = 0; i < extra_nodes; ++i) {
        pool.push_back(g.emplace_node(static_cast<int>(1000000 + i)));
    }

    // rebuild used pairs
    used_pairs.clear();
    for (auto &e : edges) {
        used_pairs.insert(
            {e.it->source()->get_data(), e.it->target()->get_data()}
        );
    }

    std::uniform_int_distribution<size_t> pool_dist(0, pool.size() - 1);
    size_t created = 0;
    while (created < extra_edges) {
        size_t si = pool_dist(rng);
        size_t ti = pool_dist(rng);
        if (si == ti) continue;
        int sv = pool[si]->get_data();
        int tv = pool[ti]->get_data();
        if (!used_pairs.insert({sv, tv}).second) continue;
        g.emplace_edge(pool[si], pool[ti], "extra");
        ++created;
    }

    g.assert_integrity();

    // Everything tracked from earlier stages must be valid
    for (auto &n : nodes) REQUIRE(n.it->get_data() == n.value);
    for (auto &e : edges) REQUIRE(e.it->get_data() == e.value);
}
