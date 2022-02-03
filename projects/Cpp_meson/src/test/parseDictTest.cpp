#include <test/doctest.h>
#include <wordle/parseDict.h>

#include <fstream>

TEST_CASE("parseDict") {
    auto fin = std::ifstream(WORDLE_DATA_DIR "/data/en_allowed.txt");

    auto words = wordle::parseDict(fin);
    CHECK(words.size() == 12973);
}
