#include "wordle/Word.h"
#include <wordle/IsWordValid.h>
#include <wordle/parseDict.h>
#include <wordle/stateFromWord.h>
#include <wordle_util.h>

#include <doctest.h>

#include <fstream>

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

constexpr bool isWordValid(Word const& guessWord, State const& state, Word const& checkWord) {
    auto b = IsSingleWordValid(guessWord, state);
    return b(checkWord);
}

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

namespace {

template <typename Op>
void forAllStates(Op&& op) {
    static_assert(NumCharacters == 5);
    // 3^5 == 243
    State s;
    for (unsigned val = 0; val < 243; ++val) {
        auto v = val;
        for (int i = 0; i < NumCharacters; ++i) {
            switch (v % 3) {
            case 0:
                s[i] = St::not_included;
                break;

            case 1:
                s[i] = St::wrong_spot;
                break;
            case 2:
                s[i] = St::correct;
                break;
            }
            v /= 3;
        }
        op(s);
    }
}

} // namespace

TEST_CASE("forAllStates") {
    std::vector<State> states;
    forAllStates([&](State const& state) {
        states.push_back(state);
    });

    REQUIRE(states.size() == 243);
    CHECK(states.front() == "00000"_state);
    CHECK(states.back() == "22222"_state);
    CHECK(states[1] == "10000"_state);
    CHECK(states[2] == "20000"_state);
    CHECK(states[3] == "01000"_state);
    CHECK(states[4] == "11000"_state);
}

// don't run that test, it takes ~18 minutes in release mode
TEST_CASE("isWordValid-bruteforce" * doctest::skip()) {
    auto fin = std::ifstream(WORDLE_DATA_DIR "/data/en_allowed.txt");
    auto allowedWords = wordle::parseDict(fin);

    fin = std::ifstream(WORDLE_DATA_DIR "/data/en_correct.txt");
    auto correctWords = wordle::parseDict(fin);

    for (auto const& correctWord : correctWords) {
        for (auto const& guessWord : correctWords) {
            auto state = stateFromWord(correctWord, guessWord);
            auto checker = IsWordValid();
            checker.addWordAndState(guessWord, state);

            for (auto const& checkWord : allowedWords) {
                bool v1 = checker(checkWord);
                bool v2 = isWordValid(guessWord, state, checkWord);
                if (v1 != v2) {
                    INFO(guessWord, " ", state, " ", checkWord);
                    CHECK(v1 == v2);
                }
            }
        }
    }
}

} // namespace wordle
