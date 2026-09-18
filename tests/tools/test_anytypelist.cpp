#include "tools/anytypelist.hpp"
#include <catch2/catch_all.hpp>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {

using list_t = tools::anytypelist;

struct entry {
    list_t::iterator it;
    std::variant<int, double, std::string> expected;
};

void check_entry(const entry &e) {
    std::visit(
        [&](const auto &v) {
            using V = std::decay_t<decltype(v)>;
            REQUIRE(e.it->template is<V>());
            REQUIRE(e.it->template get<V>() == v);
        },
        e.expected
    );
}

void check_all(const std::vector<entry> &entries) {
    for (const auto &e : entries) check_entry(e);
}

} // namespace

TEST_CASE("anytypelist default construction", "[anytypelist]") {
    list_t l;
    REQUIRE(l.empty());
    REQUIRE(l.size() == 0);
    REQUIRE(l.begin() == l.end());
}

TEST_CASE("anytypelist emplace_front and iteration", "[anytypelist]") {
    list_t l;
    auto it1 = l.emplace_front<int>(42);
    REQUIRE(l.size() == 1);
    REQUIRE_FALSE(l.empty());
    REQUIRE(it1->is<int>());
    REQUIRE(it1->get<int>() == 42);

    auto it2 = l.emplace_front<std::string>("hello");
    REQUIRE(l.size() == 2);
    REQUIRE(it2->is<std::string>());
    REQUIRE(it2->get<std::string>() == "hello");

    // The most recent insertion ends up at the front of the list
    auto it = l.begin();
    REQUIRE(it->is<std::string>());
    REQUIRE(it->get<std::string>() == "hello");
    ++it;
    REQUIRE(it->is<int>());
    REQUIRE(it->get<int>() == 42);
    ++it;
    REQUIRE(it == l.end());
}

TEST_CASE("anytypelist push_front", "[anytypelist]") {
    list_t l;
    l.push_front(42);
    l.push_front(std::string("hello"));
    REQUIRE(l.size() == 2);
    auto it = l.begin();
    REQUIRE(it->is<std::string>());
    REQUIRE(it->get<std::string>() == "hello");
    ++it;
    REQUIRE(it->is<int>());
    REQUIRE(it->get<int>() == 42);
    ++it;
    REQUIRE(it == l.end());
}

TEST_CASE("anytypelist erase from begin", "[anytypelist]") {
    list_t l;
    l.emplace_front<int>(1);
    l.emplace_front<double>(2.5);
    l.emplace_front<std::string>("three");
    // Order from begin: string, double, int
    REQUIRE(l.size() == 3);
    auto it = l.begin();
    REQUIRE(it->is<std::string>());
    it = l.erase(it);
    REQUIRE(l.size() == 2);
    REQUIRE(it->is<double>());
    REQUIRE(it->get<double>() == 2.5);

    it = l.erase(it);
    REQUIRE(l.size() == 1);
    REQUIRE(it->is<int>());
    REQUIRE(it->get<int>() == 1);

    it = l.erase(it);
    REQUIRE(l.size() == 0);
    REQUIRE(it == l.end());
    REQUIRE(l.empty());
}

TEST_CASE("anytypelist erase from middle", "[anytypelist]") {
    list_t l;
    auto it1 = l.emplace_front<int>(1);
    auto it2 = l.emplace_front<int>(2);
    auto it3 = l.emplace_front<int>(3);
    auto it4 = l.emplace_front<int>(4);
    // Order from begin: 4, 3, 2, 1

    // Erase the element that is neither at the front nor at the back
    auto next = l.erase(it2);
    REQUIRE(l.size() == 3);
    REQUIRE(next->is<int>());
    REQUIRE(next->get<int>() == 1);

    // Every other iterator still points to its original value
    REQUIRE(it1->get<int>() == 1);
    REQUIRE(it3->get<int>() == 3);
    REQUIRE(it4->get<int>() == 4);
}

TEST_CASE("anytypelist erase from end", "[anytypelist]") {
    list_t l;
    auto it1 = l.emplace_front<int>(1);
    auto it2 = l.emplace_front<int>(2);
    // Order from begin: 2, 1

    auto next = l.erase(it1);
    REQUIRE(next == l.end());
    REQUIRE(l.size() == 1);
    REQUIRE(it2->get<int>() == 2);
}

TEST_CASE("anytypelist erase preserves other iterators", "[anytypelist]") {
    list_t l;
    auto a = l.emplace_front<int>(10);
    auto b = l.emplace_front<int>(20);
    auto c = l.emplace_front<int>(30);
    auto d = l.emplace_front<int>(40);

    l.erase(b);

    REQUIRE(a->get<int>() == 10);
    REQUIRE(c->get<int>() == 30);
    REQUIRE(d->get<int>() == 40);
}

TEST_CASE("anytypelist clear", "[anytypelist]") {
    list_t l;
    l.emplace_front<int>(1);
    l.emplace_front<int>(2);
    l.clear();
    REQUIRE(l.empty());
    REQUIRE(l.size() == 0);
    REQUIRE(l.begin() == l.end());
}

TEST_CASE("anytypelist move constructor", "[anytypelist]") {
    list_t l1;
    l1.emplace_front<int>(42);
    l1.emplace_front<std::string>("hello");
    list_t l2(std::move(l1));
    REQUIRE(l2.size() == 2);
    REQUIRE_FALSE(l2.empty());
    REQUIRE(l1.empty());
    REQUIRE(l1.size() == 0);
    auto it = l2.begin();
    REQUIRE(it->is<std::string>());
    REQUIRE(it->get<std::string>() == "hello");
    ++it;
    REQUIRE(it->is<int>());
    REQUIRE(it->get<int>() == 42);
}

TEST_CASE(
    "anytypelist move constructor keeps iterators valid", "[anytypelist]"
) {
    list_t l1;
    auto it1 = l1.emplace_front<int>(1);
    auto it2 = l1.emplace_front<std::string>("two");

    list_t l2(std::move(l1));

    // Nodes are transferred without being relocated so existing iterators
    // keep pointing at the same addresses
    REQUIRE(it1->is<int>());
    REQUIRE(it1->get<int>() == 1);
    REQUIRE(it2->is<std::string>());
    REQUIRE(it2->get<std::string>() == "two");
    REQUIRE(l1.empty());
    REQUIRE(l2.size() == 2);
}

TEST_CASE("anytypelist move assignment", "[anytypelist]") {
    list_t l1;
    l1.emplace_front<int>(1);
    list_t l2;
    l2.emplace_front<double>(2.0);
    l2 = std::move(l1);
    REQUIRE(l2.size() == 1);
    REQUIRE(l2.begin()->is<int>());
    REQUIRE(l2.begin()->get<int>() == 1);
    REQUIRE(l1.empty());
    REQUIRE(l1.size() == 0);
}

TEST_CASE(
    "anytypelist move assignment keeps iterators valid", "[anytypelist]"
) {
    list_t l1;
    auto it1 = l1.emplace_front<int>(1);
    auto it2 = l1.emplace_front<int>(2);

    list_t l2;
    l2.emplace_front<std::string>("stale");
    l2 = std::move(l1);

    REQUIRE(it1->get<int>() == 1);
    REQUIRE(it2->get<int>() == 2);
    REQUIRE(l1.empty());
    REQUIRE(l2.size() == 2);
}

TEST_CASE("anytypelist move assignment to itself", "[anytypelist]") {
    list_t l1;
    l1.emplace_front<int>(1);
    l1.emplace_front<double>(2.0);
    list_t &ref = l1;
    l1 = std::move(ref);
    REQUIRE(l1.size() == 2);

    auto it = l1.begin();
    REQUIRE(it->is<double>());
    REQUIRE(it->get<double>() == 2.0);
    ++it;
    REQUIRE(it->is<int>());
    REQUIRE(it->get<int>() == 1);
}

TEST_CASE("anytypelist const correctness", "[anytypelist]") {
    list_t l;
    l.emplace_front<int>(42);
    const list_t &cl = l;
    auto cit = cl.begin();
    REQUIRE(cit->is<int>());
    REQUIRE(cit->get<int>() == 42);
    const auto &node = *cit;
    REQUIRE(node.get<int>() == 42);
}

TEST_CASE("anytypelist iterator conversion", "[anytypelist]") {
    list_t l;
    l.emplace_front<int>(42);
    list_t::iterator it = l.begin();
    list_t::const_iterator cit = it;
    REQUIRE(cit->is<int>());
    REQUIRE(cit->get<int>() == 42);
}

TEST_CASE("anytypelist iterator operations", "[anytypelist]") {
    list_t l;
    const list_t &cl = l;
    l.emplace_front<int>(1);
    l.emplace_front<int>(2);
    auto it = l.begin();
    auto cit = cl.begin();
    REQUIRE(it->get<int>() == 2);
    REQUIRE(cit->get<int>() == 2);
    ++it;
    ++cit;
    REQUIRE(it->get<int>() == 1);
    REQUIRE(cit->get<int>() == 1);
    it++;
    cit++;
    REQUIRE(it == l.end());
    REQUIRE(cit == cl.end());
}

TEST_CASE("anytypelist emplace_front with multiple args", "[anytypelist]") {
    struct Foo {
        int a;
        std::string b;
        Foo(int a, std::string b) : a(a), b(b) {}
    };
    list_t l;
    l.emplace_front<Foo>(1, "bar");
    REQUIRE(l.size() == 1);
    auto it = l.begin();
    REQUIRE(it->is<Foo>());
    REQUIRE(it->get<Foo>().a == 1);
    REQUIRE(it->get<Foo>().b == "bar");
}

TEST_CASE(
    "anytypelist emplace_front does not invalidate iterators", "[anytypelist]"
) {
    list_t l;
    auto it1 = l.emplace_front<int>(1);
    auto it2 = l.emplace_front<int>(2);
    REQUIRE(it1->get<int>() == 1);
    REQUIRE(it2->get<int>() == 2);
    l.emplace_front<int>(3);
    REQUIRE(it1->get<int>() == 1);
    REQUIRE(it2->get<int>() == 2);
}

TEST_CASE("anytypelist is not copyable", "[anytypelist]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<list_t>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<list_t>);
}

TEST_CASE("anytypelist push_front with lvalue and rvalue", "[anytypelist]") {
    list_t l;
    int x = 42;
    l.push_front(x);
    l.push_front(43);
    REQUIRE(l.size() == 2);
    auto it = l.begin();
    REQUIRE(it->get<int>() == 43);
    ++it;
    REQUIRE(it->get<int>() == 42);
}

TEST_CASE("anytypelist many mixed nodes survive erasure", "[anytypelist]") {
    list_t l;
    std::vector<entry> entries;
    const int n = 90;

    // Insert a mix of int, double and std::string nodes at the front
    for (int i = 0; i < n; ++i) {
        switch (i % 3) {
        case 0: {
            auto it = l.emplace_front<int>(i);
            entries.push_back({it, i});
            break;
        }
        case 1: {
            auto it = l.emplace_front<double>(i + 0.5);
            entries.push_back({it, i + 0.5});
            break;
        }
        case 2: {
            auto it = l.emplace_front<std::string>(std::to_string(i));
            entries.push_back({it, std::to_string(i)});
            break;
        }
        }
    }

    REQUIRE(l.size() == n);
    check_all(entries);

    // Erase every second tracked element. The remaining iterators are moved
    // into a separate list so that the erased ones are never dereferenced
    // again after their node is gone
    std::vector<entry> remaining;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i % 2 == 0) {
            l.erase(entries[i].it);
        } else {
            remaining.push_back(entries[i]);
        }
    }

    REQUIRE(l.size() == remaining.size());

    // Every surviving iterator must still point to its original value
    check_all(remaining);
}

TEST_CASE(
    "anytypelist iterators survive interleaved inserts and erases",
    "[anytypelist]"
) {
    list_t l;
    std::vector<entry> kept;

    // Insert the first batch
    for (int i = 0; i < 30; ++i) {
        auto it = l.emplace_front<int>(i);
        kept.push_back({it, i});
    }
    check_all(kept);

    // Erase every third tracked element from the middle of the list
    std::vector<entry> survivors;
    for (size_t i = 0; i < kept.size(); ++i) {
        if (i % 3 == 0) {
            l.erase(kept[i].it);
        } else {
            survivors.push_back(kept[i]);
        }
    }
    kept = std::move(survivors);
    check_all(kept);

    // Insert a fresh batch at the front and verify that every previously
    // kept iterator is still valid and still points to the same value
    for (int i = 100; i < 130; ++i) {
        auto it = l.emplace_front<int>(i);
        kept.push_back({it, i});
    }
    check_all(kept);
    REQUIRE(l.size() == kept.size());
}
