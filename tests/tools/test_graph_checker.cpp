#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>

// The checker tests need to corrupt the graph on purpose
#define private public
#include "tools/graph.hpp"
#undef private

using graph_t = tools::orgraph_t<int, std::string>;
using node_iter = graph_t::node_iter;

TEST_CASE("graph checker accepts well formed graphs", "[graph][checker]") {
    SECTION("empty graph") {
        graph_t g;
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("graph with isolated nodes") {
        graph_t g;
        for (int i = 0; i < 10; ++i) g.emplace_node(i);
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("chain") {
        graph_t g;
        auto a = g.emplace_node(1);
        auto b = g.emplace_node(2);
        auto c = g.emplace_node(3);
        g.emplace_edge(a, b, "ab");
        g.emplace_edge(b, c, "bc");
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("cycle") {
        graph_t g;
        auto a = g.emplace_node(1);
        auto b = g.emplace_node(2);
        auto c = g.emplace_node(3);
        g.emplace_edge(a, b, "ab");
        g.emplace_edge(b, c, "bc");
        g.emplace_edge(c, a, "ca");
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("edge into the itself") {
        graph_t g;
        auto a = g.emplace_node(1);
        g.emplace_edge(a, a, "aa");
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("pair of opposite edges") {
        graph_t g;
        auto a = g.emplace_node(1);
        auto b = g.emplace_node(2);
        g.emplace_edge(a, b, "ab");
        g.emplace_edge(b, a, "ba");
        REQUIRE_NOTHROW(g.assert_integrity());
    }

    SECTION("after every structural mutation") {
        graph_t g;
        auto a = g.emplace_node(1);
        auto b = g.emplace_node(2);
        auto c = g.emplace_node(3);
        auto e = g.emplace_edge(a, b, "ab");
        REQUIRE_NOTHROW(g.assert_integrity());
        g.set_egde_target(e, c);
        REQUIRE_NOTHROW(g.assert_integrity());
        g.set_egde_source(e, c);
        REQUIRE_NOTHROW(g.assert_integrity());
        g.remove_edge(e);
        REQUIRE_NOTHROW(g.assert_integrity());
        g.remove_node(a);
        REQUIRE_NOTHROW(g.assert_integrity());
    }
}

TEST_CASE("graph checker rejects ill-formed edge", "[graph][checker]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    auto c = g.emplace_node(3);
    auto e = g.emplace_edge(a, b, "ab");
    REQUIRE_NOTHROW(g.assert_integrity());

    SECTION("edge is present in lists of two nodes") {
        SECTION("in forward list") {
            c->forward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
        SECTION("in backward list") {
            c->backward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
    }
    SECTION("edge is duplicated in a list of a single node") {
        SECTION("in forward list") {
            a->forward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
        SECTION("in backward list") {
            b->backward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
    }
    SECTION("edge is attached to a wrong node") {
        SECTION("in forward list") {
            a->forward_list.clear();
            c->forward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
        SECTION("in backward list") {
            b->backward_list.clear();
            c->backward_list.push_back(e);
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
    }
    SECTION("edge is orphan") {
        SECTION("by forward list") {
            a->forward_list.clear();
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
        SECTION("by backward list") {
            b->backward_list.clear();
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
        SECTION("by all lists") {
            a->forward_list.clear();
            b->backward_list.clear();
            REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
        }
    }
    SECTION("edge ends is flipped") {
        using std::swap;
        swap(e->source_node, e->target_node);
        REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
    }
}

TEST_CASE("graph checker rejects invalidated edge", "[graph][checker]") {
    graph_t g;
    auto a = g.emplace_node(1);
    auto b = g.emplace_node(2);
    g.emplace_edge(a, b, "ab");
    REQUIRE_NOTHROW(g.assert_integrity());

    // The edge is removed from global edge container, however remains in
    // one of forward lists
    g.edges_list.clear();

    SECTION("in forward list") {
        b->backward_list.clear();
        REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
    }

    SECTION("in backward list") {
        a->forward_list.clear();
        REQUIRE_THROWS_AS(g.assert_integrity(), std::runtime_error);
    }
}
