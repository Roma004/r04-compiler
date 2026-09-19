#include "tools/blocklist.hpp"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace {

// Collect every element of the container into a sorted vector so that tests
// do not depend on the internal ordering of blocks or of slots inside a block
template <typename Container> auto collect_sorted(const Container &c) {
    using value_type = typename Container::value_type;
    std::vector<value_type> out;
    for (auto it = c.begin(); it != c.end(); ++it) out.push_back(*it);
    std::sort(out.begin(), out.end());
    return out;
}

struct tracker {
    explicit tracker(int i = 0) : id(i) { ++alive; }
    tracker(const tracker &o) : id(o.id) { ++alive; }
    tracker(tracker &&o) noexcept : id(o.id) { ++alive; }
    ~tracker() { --alive; }

    tracker &operator=(const tracker &) = default;
    tracker &operator=(tracker &&) = default;
    bool operator==(const tracker &o) const { return id == o.id; }
    bool operator<(const tracker &o) const { return id < o.id; }

    static int alive;
    int id;
};
int tracker::alive = 0;

#define DECLARE_TEST_STRUCTURE(                                           \
    __name__,                                                             \
    def_construct,                                                        \
    copy_construct,                                                       \
    move_construct,                                                       \
    copy_assign,                                                          \
    move_assign                                                           \
)                                                                         \
    struct __name__ {                                                     \
        int val;                                                          \
        __name__() = def_construct;                                       \
        __name__(const __name__ &) = copy_construct;                      \
        __name__(__name__ &&) noexcept = move_construct;                  \
        __name__ &operator=(const __name__ &) = copy_assign;              \
        __name__ &operator=(__name__ &&) noexcept = move_assign;          \
        explicit __name__(int v) : val(v) {}                              \
        bool operator==(const __name__ &o) const { return val == o.val; } \
        bool operator<(const __name__ &o) const { return val < o.val; }   \
    };

DECLARE_TEST_STRUCTURE(
    no_default_constructible,
    /* def_construct  */ delete,
    /* copy_construct */ default,
    /* move_construct */ default,
    /* copy_assign    */ default,
    /* move_assign    */ default
);
DECLARE_TEST_STRUCTURE(
    non_assignable,
    /* def_construct  */ default,
    /* copy_construct */ default,
    /* move_construct */ default,
    /* copy_assign    */ delete,
    /* move_assign    */ delete
);
DECLARE_TEST_STRUCTURE(
    non_copyable,
    /* def_construct  */ default,
    /* copy_construct */ delete,
    /* move_construct */ default,
    /* copy_assign    */ delete,
    /* move_assign    */ default
);
DECLARE_TEST_STRUCTURE(
    only_moveable,
    /* def_construct  */ delete,
    /* copy_construct */ delete,
    /* move_construct */ default,
    /* copy_assign    */ delete,
    /* move_assign    */ default
);

struct with_reference {
    with_reference() = delete;
    explicit with_reference(int &v) : val(v) {}
    with_reference(const with_reference &) = default;
    with_reference(with_reference &&) noexcept = default;

    with_reference &operator=(const with_reference &) = delete;
    with_reference &operator=(with_reference &&) noexcept = delete;
    bool operator==(const with_reference &o) const { return val == o.val; }
    bool operator<(const with_reference &o) const { return val < o.val; }

    int &val;
};

template <size_t N> struct SizeTag {
    static constexpr size_t value = N;
};

} // namespace

TEMPLATE_TEST_CASE(
    "blocklist behaviour",
    "[blocklist]",
    SizeTag<8>,
    SizeTag<16>,
    SizeTag<32>,
    SizeTag<64>
) {
    constexpr size_t blk = TestType::value;
    using list_t = tools::blocklist<int, blk>;

    SECTION("default constructed container is empty") {
        list_t l;
        REQUIRE(l.empty());
        REQUIRE(l.size() == 0);
        REQUIRE(l.begin() == l.end());
    }

    SECTION("emplace inserts an element and returns an iterator to it") {
        list_t l;
        auto it = l.emplace(42);
        REQUIRE_FALSE(l.empty());
        REQUIRE(l.size() == 1);
        REQUIRE(*it == 42);
        REQUIRE(l.begin() != l.end());
    }

    SECTION("insert copies an lvalue") {
        list_t l;
        int x = 7;
        auto it = l.insert(x);
        REQUIRE(*it == 7);
        REQUIRE(l.size() == 1);
    }

    SECTION("insert moves an rvalue") {
        list_t l;
        auto it = l.insert(13);
        REQUIRE(*it == 13);
        REQUIRE(l.size() == 1);
    }

    SECTION("size tracks insertions and erasures") {
        list_t l;
        for (int i = 0; i < 5; ++i) l.emplace(i);
        REQUIRE(l.size() == 5);
        auto it = l.begin();
        it = l.erase(it);
        REQUIRE(l.size() == 4);
    }

    SECTION("iteration visits every element exactly once") {
        list_t l;
        for (int i = 1; i <= 5; ++i) l.emplace(i);
        REQUIRE(collect_sorted(l) == std::vector<int>{1, 2, 3, 4, 5});
    }

    SECTION("iteration works across several blocks") {
        list_t l;
        const int n = blk * 3 + 5;
        for (int i = 0; i < n; ++i) l.emplace(i);
        REQUIRE(l.size() == n);
        std::vector<int> expected;
        for (int i = 0; i < n; ++i) expected.push_back(i);
        REQUIRE(collect_sorted(l) == expected);
    }

    SECTION("erase removes the pointed element") {
        list_t l;
        auto it1 = l.emplace(1);
        l.emplace(2);
        l.emplace(3);
        l.erase(it1);
        REQUIRE(collect_sorted(l) == std::vector<int>{2, 3});
        REQUIRE(l.size() == 2);
    }

    SECTION("repeatedly erasing the begin iterator empties the container") {
        list_t l;
        for (int i = 0; i < 10; ++i) l.emplace(i);
        while (!l.empty()) l.erase(l.begin());
        REQUIRE(l.empty());
        REQUIRE(l.size() == 0);
        REQUIRE(l.begin() == l.end());
    }

    SECTION("erasing the last element yields end") {
        list_t l;
        auto it = l.emplace(1);
        REQUIRE(l.erase(it) == l.end());
        REQUIRE(l.empty());
    }

    SECTION("insertion does not invalidate existing iterators") {
        list_t l;
        auto it1 = l.emplace(100);
        auto it2 = l.emplace(200);
        auto it3 = l.emplace(300);
        // Force creation of several additional blocks
        for (int i = 0; i < blk * 2; ++i) l.emplace(i);
        REQUIRE(*it1 == 100);
        REQUIRE(*it2 == 200);
        REQUIRE(*it3 == 300);
    }

    SECTION("erasure does not invalidate iterators to other elements") {
        list_t l;
        auto it1 = l.emplace(10);
        auto it2 = l.emplace(20);
        auto it3 = l.emplace(30);
        l.erase(it2);
        REQUIRE(*it1 == 10);
        REQUIRE(*it3 == 30);
    }

    SECTION(
        "erasing an element keeps iterators to earlier insertions valid across "
        "blocks"
    ) {
        list_t l;
        for (int i = 0; i < blk * 3; ++i) l.emplace(10000 - i);
        auto anchor = l.emplace(1);
        for (int i = 0; i < blk * 3; ++i) l.emplace(10000 - i);
        auto it = l.begin();
        while (it != l.end()) {
            if (it == anchor) {
                ++it;
                continue;
            }
            it = l.erase(it);
        }
        REQUIRE(l.size() == 1);
        REQUIRE(*anchor == 1);
    }

    SECTION("destruction destroys every stored element") {
        tracker::alive = 0;
        {
            tools::blocklist<tracker, blk> l;
            l.emplace(1);
            l.emplace(2);
            l.emplace(3);
            REQUIRE(tracker::alive == 3);
        }
        REQUIRE(tracker::alive == 0);
    }

    SECTION("erasing destroys only the erased element") {
        tracker::alive = 0;
        {
            tools::blocklist<tracker, blk> l;
            l.emplace(1);
            l.emplace(2);
            l.emplace(3);
            l.erase(l.begin());
            REQUIRE(tracker::alive == 2);
        }
        REQUIRE(tracker::alive == 0);
    }

    SECTION("works with non trivial value types") {
        tools::blocklist<std::string, blk> l;
        l.emplace("alpha");
        l.emplace("beta");
        l.emplace("gamma");
        REQUIRE(
            collect_sorted(l)
            == std::vector<std::string>{"alpha", "beta", "gamma"}
        );
    }

    SECTION("iterator is convertible to const_iterator") {
        list_t l;
        l.emplace(7);
        typename list_t::iterator it = l.begin();
        typename list_t::const_iterator cit = it;
        REQUIRE(*cit == 7);
    }

    SECTION("const iteration yields every element") {
        list_t l;
        l.emplace(1);
        l.emplace(2);
        l.emplace(3);
        const list_t &cl = l;
        REQUIRE(collect_sorted(cl) == std::vector<int>{1, 2, 3});
    }

    SECTION("prefix increment returns a reference to the same iterator") {
        list_t l;
        l.emplace(1);
        l.emplace(2);
        auto it = l.begin();
        auto &ref = ++it;
        REQUIRE(&ref == &it);
        REQUIRE(l.size() == 2);
    }

    SECTION("postfix increment keeps a usable copy of the previous position") {
        list_t l;
        l.emplace(1);
        l.emplace(2);
        auto it = l.begin();
        auto prev = it++;
        REQUIRE(prev != it);
        REQUIRE(l.size() == 2);
    }

    SECTION(
        "many elements spanning several blocks survive mixed insert and erase"
    ) {
        list_t l;
        const int n = blk * 4;
        std::vector<std::pair<typename list_t::iterator, int>> its;
        its.reserve(n);
        for (int i = 0; i < n; ++i) its.emplace_back(l.emplace(i), i);
        REQUIRE(l.size() == n);

        // Erase every second element. The tracking array is kept in sync with
        // the surviving elements: as soon as an element is erased the
        // iterator pointing to it is removed from the array as well, so the
        // array only ever holds valid iterators
        for (int i = n - 1; i >= 0; i -= 2) {
            l.erase(its[i].first);
            its.erase(its.begin() + i);
        }

        REQUIRE(l.size() == its.size());

        // Every iterator left in the tracking array must still point to the
        // exact value that was inserted through it
        for (int i = 0; i < its.size(); ++i) {
            REQUIRE(*its[i].first == its[i].second);
        }
    }

    SECTION("clear state is restored after erasing everything") {
        list_t l;
        for (int i = 0; i < blk * 2 + 1; ++i) l.emplace(i);
        while (!l.empty()) l.erase(l.begin());
        REQUIRE(l.empty());
        REQUIRE(l.size() == 0);
        REQUIRE(l.begin() == l.end());
        // Subsequent insertions still work
        auto it = l.emplace(99);
        REQUIRE(*it == 99);
        REQUIRE(l.size() == 1);
    }

    SECTION("move constructor keeps previously issued iterators valid") {
        list_t l1;
        std::vector<typename list_t::iterator> its;
        const int n = 30;
        its.reserve(n);
        for (int i = 0; i < n; ++i) its.push_back(l1.emplace(i));
        REQUIRE(l1.size() == n);

        list_t l2(std::move(l1));

        REQUIRE(l2.size() == n);
        for (int i = 0; i < n; ++i) REQUIRE(*its[i] == i);
    }

    SECTION("move assignment keeps previously issued iterators valid") {
        list_t l1;
        std::vector<typename list_t::iterator> its;
        const int n = 30;
        its.reserve(n);
        for (int i = 0; i < n; ++i) its.push_back(l1.emplace(i));
        REQUIRE(l1.size() == n);

        list_t l2 = std::move(l1);

        REQUIRE(l2.size() == n);
        for (int i = 0; i < n; ++i) REQUIRE(*its[i] == i);
    }
}

TEMPLATE_TEST_CASE(
    "blocklist of structures with construct and assign restrictions",
    "[blocklist]",
    no_default_constructible,
    non_assignable,
    non_copyable,
    only_moveable,
    with_reference
) {
    constexpr size_t blk = 8;
    constexpr size_t nblks = 10;
    using list_t = tools::blocklist<TestType, blk>;

    // test expects reference type to be contructed from int& and non-reference
    // from int. So make a list of lvalue integer to satisfy them all
    std::vector<int> ref_values;
    for (int i = 0; i < blk * nblks; ++i) { ref_values.emplace_back(i); }

    // type `with_reference` can't be stored inside std::vector, so we can't
    // use `collect_sorted`
    auto get_values = [](auto &conteiner) {
        std::vector<int> res;
        res.reserve(conteiner.size());
        for (auto &el : conteiner) res.push_back(el.val);
        std::sort(res.begin(), res.end());
        return res;
    };

    SECTION("Type can be stored") {
        list_t l;
        for (auto &el : ref_values) l.emplace(el);

        REQUIRE(l.capacity() == ref_values.size());
        REQUIRE(get_values(l) == ref_values);
    }

    SECTION("types survive erasure and reuse") {
        list_t l;
        std::vector<std::pair<int, typename list_t::iterator>> its;
        std::vector<int> ref;

        // fill list with reference values
        for (auto &el : ref_values) {
            its.emplace_back(el, l.emplace(el));
            ref.push_back(el);
        }

        // requires maximal elements packing
        REQUIRE(l.capacity() == blk * nblks);
        REQUIRE(get_values(l) == ref);

        // remove elements 0, 2, 4, 6, ...
        for (int i = 0; i < its.size(); ++i) {
            l.erase(its[i].second);
            its.erase(its.begin() + i);
            ref.erase(ref.begin() + i);
        }

        // elements now are spreded inside the same capacity
        REQUIRE(l.capacity() == blk * nblks);
        REQUIRE(get_values(l) == ref);

        // add new elements
        for (int i = 0; i < ref_values.size() / 2; ++i) {
            l.emplace(ref_values[i]);
            ref.push_back(ref_values[i]);
        }
        std::sort(ref.begin(), ref.end());

        // packing must be maximal again
        REQUIRE(l.capacity() == ref.size());
        REQUIRE(get_values(l) == ref);

        // require initially inserted iterators are still valid
        for (auto &[el, it] : its) { REQUIRE(it->val == el); }
    }
}
