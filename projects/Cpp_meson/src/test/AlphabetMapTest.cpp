#include <wordle/AlphabetMap.h>

#include <doctest.h>

#include <cstdint>

namespace wordle {

TEST_CASE("AlphabetMap-bool") {
    auto bm = AlphabetMap<bool>(true);
    for (char c = 'a'; c != 'z'; ++c) {
        CHECK(bm[c - 'a'] == true);
    }
}

TEST_CASE("AlphabetMap-uint8_t") {
    auto bm = AlphabetMap<uint8_t>();
    for (char c = 'a'; c != 'z'; ++c) {
        CHECK(bm[c - 'a'] == 0);
    }

    auto c = 'f' - 'a';
    bm[c] += 7;
    CHECK(bm[c] == 7);
    CHECK(bm[c - 1] == 0);
    CHECK(bm[c + 1] == 0);
}

} // namespace wordle
