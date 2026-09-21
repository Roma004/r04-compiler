#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <string>

#include "tools/tree.hpp"

using graph_t = tools::orgraph_t<int, std::string>;
using tree_t = tools::tree_t<int, std::string>;
using tree_node_iter = tree_t::node_iter;

TEST_CASE("tree checker accepts well formed trees", "[tree][checker]") {
    SECTION("no nodes") {
        tree_t t;
        REQUIRE_NOTHROW(t.assert_integrity());
    }

    SECTION("single node") {
        tree_t t;
        auto r = t.emplace_node(1);
        t.set_root(r);
        REQUIRE_NOTHROW(t.assert_integrity());
    }

    SECTION("chain") {
        tree_t t;
        auto r = t.emplace_node(1);
        auto a = t.emplace_node(2);
        auto b = t.emplace_node(3);
        auto c = t.emplace_node(4);
        t.set_root(r);
        t.emplace_edge(r, a, "ra");
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(b, c, "bc");
        REQUIRE_NOTHROW(t.assert_integrity());
    }

    SECTION("star") {
        tree_t t;
        auto r = t.emplace_node(0);
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        t.set_root(r);
        t.emplace_edge(r, a, "ra");
        t.emplace_edge(r, b, "rb");
        t.emplace_edge(r, c, "rc");
        REQUIRE_NOTHROW(t.assert_integrity());
    }

    SECTION("deep tree with several branches") {
        tree_t t;
        auto r = t.emplace_node(0);
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        auto d = t.emplace_node(4);
        auto e = t.emplace_node(5);
        auto f = t.emplace_node(6);
        t.set_root(r);
        t.emplace_edge(r, a, "ra");
        t.emplace_edge(r, b, "rb");
        t.emplace_edge(a, c, "ac");
        t.emplace_edge(a, d, "ad");
        t.emplace_edge(b, e, "be");
        t.emplace_edge(c, f, "cf");
        REQUIRE_NOTHROW(t.assert_integrity());
    }
}

TEST_CASE("tree checker rejects cycles", "[tree][checker]") {
    SECTION("two node cycle") {
        tree_t t;
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        t.set_root(a);
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(b, a, "ba");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }

    SECTION("three node ring") {
        tree_t t;
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        t.set_root(a);
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(b, c, "bc");
        t.emplace_edge(c, a, "ca");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }

    SECTION("longer ring") {
        tree_t t;
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        auto d = t.emplace_node(4);
        t.set_root(a);
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(b, c, "bc");
        t.emplace_edge(c, d, "cd");
        t.emplace_edge(d, a, "da");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }

    SECTION("self loop") {
        tree_t t;
        auto a = t.emplace_node(1);
        t.set_root(a);
        t.emplace_edge(a, a, "aa");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }
}

TEST_CASE("tree checker rejects merged branches", "[tree][checker]") {
    SECTION("diamond") {
        // Two distinct paths from the root converge at a single node
        tree_t t;
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        auto d = t.emplace_node(4);
        t.set_root(a);
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(a, c, "ac");
        t.emplace_edge(b, d, "bd");
        t.emplace_edge(c, d, "cd");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }

    SECTION("node with two parents") {
        // A node is reachable both directly from the root and through a
        // sibling of the direct child
        tree_t t;
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        t.set_root(a);
        t.emplace_edge(a, b, "ab");
        t.emplace_edge(a, c, "ac");
        t.emplace_edge(b, c, "bc");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }

    SECTION("two subtrees glued at the bottom") {
        // The two long branches share their last node
        tree_t t;
        auto r = t.emplace_node(0);
        auto a = t.emplace_node(1);
        auto b = t.emplace_node(2);
        auto c = t.emplace_node(3);
        auto d = t.emplace_node(4);
        auto e = t.emplace_node(5);
        t.set_root(r);
        t.emplace_edge(r, a, "ra");
        t.emplace_edge(r, b, "rb");
        t.emplace_edge(a, c, "ac");
        t.emplace_edge(b, d, "bd");
        t.emplace_edge(c, e, "ce");
        t.emplace_edge(d, e, "de");
        REQUIRE_THROWS_AS(t.assert_integrity(), std::runtime_error);
    }
}
