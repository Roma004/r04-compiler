#include "tools/fixedset.hpp"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

// Collect all elements of a fixedset into a sorted vector so that tests do not
// depend on slot ordering inside the container
template <typename Set> auto collect_sorted(const Set &s) {
    using value_type = typename Set::value_type;
    std::vector<value_type> out;
    for (auto it = s.begin(); it != s.end(); ++it)
        out.push_back(*it);
    std::sort(out.begin(), out.end());
    return out;
}
template <typename Set> auto collect_sorted(Set &s) {
    return collect_sorted(const_cast<const Set &>(s));
}

struct tracker {
    static int alive;
    int id;
    explicit tracker(int i = 0) : id(i) { ++alive; }
    tracker(const tracker &o) : id(o.id) { ++alive; }
    tracker(tracker &&o) noexcept : id(o.id) { ++alive; }
    ~tracker() { --alive; }
    tracker &operator=(const tracker &) = default;
    tracker &operator=(tracker &&) = default;
    bool operator==(const tracker &o) const { return id == o.id; }
    bool operator<(const tracker &o) const { return id < o.id; }
};
int tracker::alive = 0;

template <size_t N> struct SizeTag {
    static constexpr size_t value = N;
};

} // namespace

TEMPLATE_TEST_CASE(
    "fixedset behaviour",
    "[fixedset]",
    SizeTag<8>,
    SizeTag<16>,
    SizeTag<32>,
    SizeTag<64>
) {
    constexpr size_t blk = TestType::value;
    using set_t = tools::fixedset<int, blk>;

    SECTION("default constructed set is empty") {
        set_t s;
        REQUIRE(s.empty());
        REQUIRE_FALSE(s.full());
        REQUIRE(s.size() == 0);
        REQUIRE(s.capacity() == blk);
        REQUIRE(s.begin() == s.end());
    }

    SECTION("emplace inserts an element and returns an iterator to it") {
        set_t s;
        auto it = s.emplace(42);
        REQUIRE_FALSE(s.empty());
        REQUIRE(s.size() == 1);
        REQUIRE(*it == 42);
        REQUIRE(s.begin() != s.end());
        REQUIRE(collect_sorted(s) == std::vector<int>{42});
    }

    SECTION("insert copies an lvalue") {
        set_t s;
        int x = 7;
        auto it = s.insert(x);
        REQUIRE(*it == 7);
        REQUIRE(s.size() == 1);
    }

    SECTION("insert moves an rvalue") {
        set_t s;
        auto it = s.insert(13);
        REQUIRE(*it == 13);
        REQUIRE(s.size() == 1);
    }

    SECTION("size grows with each insertion") {
        set_t s;
        for (size_t i = 0; i < 5; ++i) s.emplace(static_cast<int>(i));
        REQUIRE(s.size() == 5);
        REQUIRE(collect_sorted(s) == std::vector<int>{0, 1, 2, 3, 4});
    }

    SECTION("set becomes full at capacity and rejects further inserts") {
        set_t s;
        for (size_t i = 0; i < blk; ++i) s.emplace(static_cast<int>(i));
        REQUIRE(s.full());
        REQUIRE(s.size() == blk);
        REQUIRE_THROWS_AS(s.emplace(0), std::length_error);
        REQUIRE_THROWS_AS(s.insert(0), std::length_error);
    }

    SECTION("erase removes the pointed element") {
        set_t s;
        auto it = s.emplace(5);
        s.emplace(6);
        s.erase(it);
        REQUIRE(s.size() == 1);
        REQUIRE(collect_sorted(s) == std::vector<int>{6});
    }

    SECTION("erase returns iterator to a remaining element or end") {
        set_t s;
        s.emplace(1);
        s.emplace(2);
        s.emplace(3);
        auto it = s.begin();
        it = s.erase(it);
        while (it != s.end()) it = s.erase(it);
        REQUIRE(s.empty());
    }

    SECTION("erase of the last element yields end") {
        set_t s;
        auto it = s.emplace(1);
        REQUIRE(s.erase(it) == s.end());
        REQUIRE(s.empty());
    }

    SECTION("erase of an invalid iterator throws runtime_error") {
        set_t s;
        auto it = s.emplace(1);
        s.erase(it);
        REQUIRE_THROWS_AS(s.erase(it), std::runtime_error);
    }

    SECTION("copy constructor produces an equal independent set") {
        set_t a;
        a.emplace(1);
        a.emplace(2);
        a.emplace(3);
        set_t b(a);
        REQUIRE(collect_sorted(b) == collect_sorted(a));
        b.erase(b.begin());
        REQUIRE(a.size() == 3);
        REQUIRE(b.size() == 2);
    }

    SECTION("copy assignment replaces contents") {
        set_t a;
        a.emplace(1);
        a.emplace(2);
        set_t b;
        b.emplace(9);
        b = a;
        REQUIRE(a.size() == 2);
        REQUIRE(b.size() == 2);
        REQUIRE(collect_sorted(b) == std::vector<int>{1, 2});
    }

    SECTION("content not changes after copy") {
        set_t a;
        a.emplace(1);
        set_t b = a;
        set_t c(a);
        *b.begin() = 2;
        *c.begin() = 3;
        REQUIRE(collect_sorted(a) == std::vector<int>{1});
        REQUIRE(collect_sorted(b) == std::vector<int>{2});
        REQUIRE(collect_sorted(c) == std::vector<int>{3});
    }

    SECTION("self copy assignment is a no op") {
        set_t a;
        a.emplace(1);
        set_t &ref = a;
        a = ref;
        REQUIRE(collect_sorted(a) == std::vector<int>{1});
    }

    SECTION("move constructor transfers elements and leaves source empty") {
        set_t a;
        a.emplace(1);
        a.emplace(2);
        set_t b(std::move(a));
        REQUIRE(collect_sorted(b) == std::vector<int>{1, 2});
        REQUIRE(a.empty());
        REQUIRE(a.size() == 0);
    }

    SECTION("move assignment transfers elements and leaves source empty") {
        set_t a;
        a.emplace(4);
        set_t b;
        b.emplace(5);
        b = std::move(a);
        REQUIRE(collect_sorted(b) == std::vector<int>{4});
        REQUIRE(a.empty());
    }

    SECTION("self move assignment is safe") {
        set_t a;
        a.emplace(1);
        set_t &ref = a;
        a = std::move(ref);
        REQUIRE(a.size() == 1);
    }

    SECTION("swap exchanges contents") {

        set_t a;
        a.emplace(1);
        set_t b;
        b.emplace(8);
        b.emplace(9);
        a.swap(b);
        REQUIRE(collect_sorted(a) == std::vector<int>{8, 9});
        REQUIRE(collect_sorted(b) == std::vector<int>{1});
    }

    SECTION("destroying the set destroys every stored element") {
        tracker::alive = 0;
        {
            tools::fixedset<tracker, blk> s;
            s.emplace(1);
            s.emplace(2);
            s.emplace(3);
            REQUIRE(tracker::alive == 3);
        }
        REQUIRE(tracker::alive == 0);
    }

    SECTION("erasing an element destroys only that element") {
        tracker::alive = 0;
        {
            tools::fixedset<tracker, blk> s;
            s.emplace(1);
            s.emplace(2);
            auto it = s.begin();
            s.erase(it);
            REQUIRE(tracker::alive == 1);
        }
        REQUIRE(tracker::alive == 0);
    }

    SECTION("copy of a set with non trivial elements is independent") {
        tracker::alive = 0;
        {
            tools::fixedset<tracker, blk> a;
            a.emplace(1);
            a.emplace(2);
            tools::fixedset<tracker, blk> b(a);
            REQUIRE(tracker::alive == 4);
            b.erase(b.begin());
            REQUIRE(a.size() == 2);
            REQUIRE(b.size() == 1);
        }
        REQUIRE(tracker::alive == 0);
    }

    SECTION("works with non trivial value types") {
        tools::fixedset<std::string, blk> s;
        s.emplace("alpha");
        s.emplace("beta");
        s.emplace("gamma");
        auto got = collect_sorted(s);
        REQUIRE(got == std::vector<std::string>{"alpha", "beta", "gamma"});
    }

    SECTION("index access round trips through iter_at and idx_of") {
        set_t s;
        s.emplace(10);
        bool found_val = false;
        for (size_t i = 0; i < s.capacity(); ++i) {
            auto it = s.iter_at(i);
            REQUIRE(it != s.end());
            REQUIRE(s.idx_of(it) == i);
            if (s.is_valid(it)) {
                REQUIRE_FALSE(found_val);
                REQUIRE(*it == 10);
                found_val = true;
            }
        }
    }

    SECTION("iterator is convertible to const_iterator") {
        set_t s;
        s.emplace(7);
        typename set_t::iterator it = s.begin();
        typename set_t::const_iterator cit = it;
        REQUIRE(*cit == 7);
    }

    SECTION("const iteration yields the same elements") {
        set_t s;
        s.emplace(1);
        s.emplace(2);
        const set_t &cs = s;
        REQUIRE(collect_sorted(cs) == collect_sorted(s));
    }

    SECTION("iterator supports postfix increment") {
        set_t s;
        s.emplace(1);
        s.emplace(2);
        auto it = s.begin();
        auto copy = it++;
        REQUIRE(copy != it);
        REQUIRE(*copy != *it);
        REQUIRE(s.size() == 2);
    }

    SECTION("capacity is available as a constexpr") {
        STATIC_REQUIRE(set_t::capacity() == blk);
    }

    SECTION("swap works correct") {
        using std::swap;
        set_t a, b;
        for (int i = 0; i < blk; ++i) {
            a.emplace(i);
            b.emplace(i);
        }
        for (int i = 0; i < blk; ++i) {
            auto &s = i % 2 == 0 ? a : b;
            s.erase(s.iter_at(i));
        }
        set_t copy_a1 = a, copy_b1 = b;
        set_t copy_a2 = a, copy_b2 = b;

        swap(copy_a1, copy_b1);
        swap(copy_b2, copy_a2);

        REQUIRE(collect_sorted(copy_a1) == collect_sorted(copy_a2));
        REQUIRE(collect_sorted(copy_b1) == collect_sorted(copy_b2));
        REQUIRE(collect_sorted(a) == collect_sorted(copy_b1));
        REQUIRE(collect_sorted(b) == collect_sorted(copy_a1));
    }
}

TEST_CASE("fixedset of different capacities are distinct types", "[fixedset]") {
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<
            tools::fixedset<int, 8>,
            tools::fixedset<int, 16>>
    );
    STATIC_REQUIRE_FALSE(
        std::
            is_assignable_v<tools::fixedset<int, 8> &, tools::fixedset<int, 16>>
    );
}
