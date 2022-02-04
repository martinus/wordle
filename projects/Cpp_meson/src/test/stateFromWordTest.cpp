#include <wordle/stateFromWord.h>
#include <wordle_util.h>

#include <doctest.h>

namespace wordle {

constexpr State stateFromWord(std::string_view correctWord, std::string_view guessWord) {
    Word wa{};
    Word wb{};
    for (size_t i = 0; i < NumCharacters; ++i) {
        wa[i] = correctWord[i] - 'a';
        wb[i] = guessWord[i] - 'a';
    }
    return wordle::stateFromWord(wa, wb);
}

static_assert(stateFromWord("aacde", "aaaxx") == "22000"_state);
static_assert(stateFromWord("aacde", "aaxxx") == "22000"_state);
static_assert(stateFromWord("abcde", "aaxxx") == "20000"_state);
static_assert(stateFromWord("abcde", "xaaxx") == "01000"_state);
static_assert(stateFromWord("gouge", "bough") == "02220"_state);
static_assert(stateFromWord("gouge", "lento") == "01001"_state);
static_assert(stateFromWord("gouge", "raise") == "00002"_state);
static_assert(stateFromWord("jeans", "ashen") == "11011"_state);
static_assert(stateFromWord("jeans", "knelt") == "01100"_state);
static_assert(stateFromWord("jeans", "raise") == "01011"_state);
static_assert(stateFromWord("knoll", "pills") == "00120"_state);
static_assert(stateFromWord("lilac", "apian") == "00120"_state);
static_assert(stateFromWord("lilac", "mambo") == "01000"_state);
static_assert(stateFromWord("lilac", "stare") == "00100"_state);
static_assert(stateFromWord("panic", "chase") == "10100"_state);
static_assert(stateFromWord("panic", "chase") == "10100"_state);
static_assert(stateFromWord("panic", "magic") == "02022"_state);
static_assert(stateFromWord("panic", "rocky") == "00100"_state);
static_assert(stateFromWord("pleat", "becap") == "01021"_state);
static_assert(stateFromWord("pleat", "model") == "00011"_state);
static_assert(stateFromWord("pleat", "stele") == "01210"_state);
static_assert(stateFromWord("pleat", "trawl") == "10101"_state);
static_assert(stateFromWord("shark", "zanza") == "01000"_state);
static_assert(stateFromWord("solar", "abaca") == "10000"_state);
static_assert(stateFromWord("solar", "alaap") == "01020"_state);
static_assert(stateFromWord("solar", "alaap") == "01020"_state);
static_assert(stateFromWord("solar", "raise") == "11010"_state);

} // namespace wordle
