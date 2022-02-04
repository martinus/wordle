#include <wordle/parseDict.h>

#include <doctest.h>

#include <fstream>
#include <stdexcept>

constexpr wordle::Word operator""_word(char const* str, size_t s) {
    wordle::Word w{};
    if (s != w.size()) {
        throw std::runtime_error("size does not match");
    }
    for (size_t i = 0; i < w.size(); ++i) {
        w[i] = str[i] - 'a';
    }
    return w;
}

TEST_CASE("parseDict") {
    auto fin = std::ifstream(WORDLE_DATA_DIR "/data/en_allowed.txt");

    auto words = wordle::parseDict(fin);
    REQUIRE(words.size() == 12972);
    CHECK(words.front() == "cigar"_word);
    CHECK(words[8555] == "norks"_word);
    CHECK(words.back() == "zymic"_word);
}
