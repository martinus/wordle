#include <wordle/parseDict.h>

#include <test/doctest.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

constexpr auto prefix = std::string_view(WORDLE_DATA_DIR "/data/en");

TEST_CASE("parseDict") {
    std::cout << "'" << prefix << "'" << std::endl;

    
}
