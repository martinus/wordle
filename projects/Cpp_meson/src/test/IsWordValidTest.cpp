#include <wordle/IsWordValid.h>
#include <wordle/stateFromWord.h>
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

TEST_CASE("IsWordValid_2") {
    auto isWordValid = IsWordValid().addWordAndState("abcde"_word, "02200"_state);
    CHECK(isWordValid("xbcxx"_word));
    CHECK(!isWordValid("xcbxx"_word));
    CHECK(isWordValid("bbcbc"_word));
    CHECK(isWordValid("xbcxx"_word));
    CHECK(!isWordValid("xxcxx"_word));

    CHECK(!isWordValid("xbcxa"_word));
    CHECK(isWordValid("xbcxb"_word));
    CHECK(isWordValid("xbcxc"_word)); // there could be another c, we don't know yet
    CHECK(!isWordValid("xbcxd"_word));
    CHECK(!isWordValid("xbcxe"_word));
    CHECK(!isWordValid("ebcxx"_word));
}

TEST_CASE("IsWordValid_1") {
    auto isWordValid = IsWordValid().addWordAndState("abcde"_word, "00010"_state);
    CHECK(!isWordValid("abcde"_word));
}

TEST_CASE("IsWordValid-shark") {
    auto isWordValid = IsWordValid();
    isWordValid.addWordAndState("zanza"_word, "01000"_state);
    CHECK(isWordValid("shark"_word));
    CHECK(!isWordValid("quiet"_word));
    CHECK(isWordValid("axxxx"_word));
    CHECK(!isWordValid("xaxxx"_word));
    CHECK(isWordValid("xxaxx"_word));
    CHECK(isWordValid("xxxax"_word));
    CHECK(!isWordValid("xxxxa"_word));
    // CHECK(!isWordValid("shaak"_word)); // not correct, only a single a is allowed, *NOT* 2
}

///////

TEST_CASE("val") {
    CHECK(!isWordValid("awake"_word, "00000"_state, "awake"_word));
    CHECK(!isWordValid("awake"_word, "00000"_state, "focal"_word));
    CHECK(isWordValid("awake"_word, "00000"_state, "floss"_word));

    CHECK(isWordValid("abcde"_word, "02200"_state, "xbcxx"_word));
    CHECK(!isWordValid("abcde"_word, "02200"_state, "xcbxx"_word));
    CHECK(isWordValid("abcde"_word, "02200"_state, "bbcbc"_word));
    CHECK(isWordValid("abcde"_word, "02200"_state, "xbcxx"_word));
    CHECK(!isWordValid("abcde"_word, "02200"_state, "xxcxx"_word));
    CHECK(!isWordValid("abcde"_word, "02200"_state, "xbcxa"_word));
    CHECK(isWordValid("abcde"_word, "02200"_state, "xbcxb"_word));
    CHECK(isWordValid("abcde"_word, "02200"_state, "xbcxc"_word)); // there could be another c, we don't know yet
    CHECK(!isWordValid("abcde"_word, "02200"_state, "xbcxd"_word));
    CHECK(!isWordValid("abcde"_word, "02200"_state, "xbcxe"_word));
    CHECK(!isWordValid("abcde"_word, "02200"_state, "ebcxx"_word));

    CHECK(!isWordValid("abcde"_word, "00010"_state, "abcde"_word));

    CHECK(isWordValid("zanza"_word, "01000"_state, "shark"_word));
    CHECK(!isWordValid("zanza"_word, "01000"_state, "quiet"_word));
    CHECK(isWordValid("zanza"_word, "01000"_state, "axxxx"_word));
    CHECK(!isWordValid("zanza"_word, "01000"_state, "xaxxx"_word));
    CHECK(isWordValid("zanza"_word, "01000"_state, "xxaxx"_word));
    CHECK(isWordValid("zanza"_word, "01000"_state, "xxxax"_word));
    CHECK(!isWordValid("zanza"_word, "01000"_state, "xxxxa"_word));
}

} // namespace wordle
