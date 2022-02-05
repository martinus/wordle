#include <wordle/parseDict.h>
#include <wordle_util.h>

#include <doctest.h>

#include <fstream>

namespace wordle {

TEST_CASE("parseDict") {
    auto fin = std::ifstream(WORDLE_DATA_DIR "/data/en_allowed.txt");

    auto words = wordle::parseDict(fin);
    REQUIRE(words.size() == 12972);
    CHECK(words.front() == "cigar"_word);
    CHECK(words[8555] == "norks"_word);
    CHECK(words.back() == "zymic"_word);
}

} // namespace wordle
