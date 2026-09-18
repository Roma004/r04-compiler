#include <catch2/catch_all.hpp>
#include "tools/bitset.hpp"

namespace {
template <size_t N>
struct SizeTag {
    static constexpr size_t value = N;
};
}

TEMPLATE_TEST_CASE("bitset basic operations", "[bitset]",
                   SizeTag<8>, SizeTag<16>, SizeTag<32>, SizeTag<64>) {
    constexpr size_t size = TestType::value;
    using bitset_t = tools::bitset<size>;

    SECTION("default constructed bitset is empty") {
        bitset_t bs;
        REQUIRE(bs.is_empty() == true);
        REQUIRE(bs.is_full() == false);
        REQUIRE(bs.count() == 0);
        REQUIRE(bs.find_free_idx() == 0);
        REQUIRE(bs.find_used_idx() == -1);
    }

    SECTION("set and unset single bit") {
        bitset_t bs;
        bs.set(3);
        REQUIRE(bs.is_empty() == false);
        REQUIRE(bs.is_used(3) == true);
        REQUIRE(bs.is_used(2) == false);
        REQUIRE(bs.count() == 1);
        REQUIRE(bs.find_free_idx() == 0);
        REQUIRE(bs.find_used_idx() == 3);

        bs.unset(3);
        REQUIRE(bs.is_empty() == true);
        REQUIRE(bs.is_used(3) == false);
        REQUIRE(bs.count() == 0);
        REQUIRE(bs.find_free_idx() == 0);
        REQUIRE(bs.find_used_idx() == -1);
    }

    SECTION("set all bits makes it full") {
        bitset_t bs;
        for (size_t i = 0; i < size; ++i) {
            bs.set(i);
        }
        REQUIRE(bs.is_full() == true);
        REQUIRE(bs.is_empty() == false);
        REQUIRE(bs.count() == size);
        REQUIRE(bs.find_free_idx() == -1);
        REQUIRE(bs.find_used_idx() == 0);
    }

    SECTION("unset all bits makes it empty") {
        bitset_t bs;
        for (size_t i = 0; i < size; ++i) {
            bs.set(i);
        }
        for (size_t i = 0; i < size; ++i) {
            bs.unset(i);
        }
        REQUIRE(bs.is_empty() == true);
        REQUIRE(bs.is_full() == false);
        REQUIRE(bs.count() == 0);
        REQUIRE(bs.find_free_idx() == 0);
        REQUIRE(bs.find_used_idx() == -1);
    }

    SECTION("find_free_idx returns first zero bit") {
        bitset_t bs;
        bs.set(0);
        bs.set(2);
        REQUIRE(bs.find_free_idx() == 1);
        bs.set(1);
        REQUIRE(bs.find_free_idx() == 3);
    }

    SECTION("find_used_idx returns first one bit") {
        bitset_t bs;
        bs.set(5);
        bs.set(7);
        REQUIRE(bs.find_used_idx() == 5);
        bs.unset(5);
        REQUIRE(bs.find_used_idx() == 7);
    }

    SECTION("set and unset are idempotent") {
        bitset_t bs;
        bs.set(2);
        bs.set(2);
        REQUIRE(bs.count() == 1);
        bs.unset(2);
        bs.unset(2);
        REQUIRE(bs.count() == 0);
    }

    SECTION("is_used checks correct bit") {
        bitset_t bs;
        bs.set(4);
        REQUIRE(bs.is_used(4) == true);
        REQUIRE(bs.is_used(3) == false);
        REQUIRE(bs.is_used(5) == false);
    }
}
