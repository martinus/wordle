#include <wordle/IsWordValid.h>
#include <wordle_util.h>

#include <doctest.h>

namespace wordle {

TEST_CASE("IsWordValid") {
    auto isWordValid = IsWordValid();
    CHECK(isWordValid("asdfj"_word));

    isWordValid.addWordAndState("awake"_word, "00000"_state);
    CHECK(!isWordValid("awake"_word));
    CHECK(!isWordValid("focal"_word));
    CHECK(isWordValid("floss"_word));
}

} // namespace wordle
