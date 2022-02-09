#include <wordle/stateFromWord.h>
#include <wordle_util.h>

namespace wordle {

static_assert(stateFromWord("aacde"_word, "aaaxx"_word) == "22000"_state);
static_assert(stateFromWord("aacde"_word, "aaxxx"_word) == "22000"_state);
static_assert(stateFromWord("abcde"_word, "aaxxx"_word) == "20000"_state);
static_assert(stateFromWord("abcde"_word, "xaaxx"_word) == "01000"_state);
static_assert(stateFromWord("gouge"_word, "bough"_word) == "02220"_state);
static_assert(stateFromWord("gouge"_word, "lento"_word) == "01001"_state);
static_assert(stateFromWord("gouge"_word, "raise"_word) == "00002"_state);
static_assert(stateFromWord("jeans"_word, "ashen"_word) == "11011"_state);
static_assert(stateFromWord("jeans"_word, "knelt"_word) == "01100"_state);
static_assert(stateFromWord("jeans"_word, "raise"_word) == "01011"_state);
static_assert(stateFromWord("knoll"_word, "pills"_word) == "00120"_state);
static_assert(stateFromWord("lilac"_word, "apian"_word) == "00120"_state);
static_assert(stateFromWord("lilac"_word, "mambo"_word) == "01000"_state);
static_assert(stateFromWord("lilac"_word, "stare"_word) == "00100"_state);
static_assert(stateFromWord("panic"_word, "chase"_word) == "10100"_state);
static_assert(stateFromWord("panic"_word, "chase"_word) == "10100"_state);
static_assert(stateFromWord("panic"_word, "magic"_word) == "02022"_state);
static_assert(stateFromWord("panic"_word, "rocky"_word) == "00100"_state);
static_assert(stateFromWord("pleat"_word, "becap"_word) == "01021"_state);
static_assert(stateFromWord("pleat"_word, "model"_word) == "00011"_state);
static_assert(stateFromWord("pleat"_word, "stele"_word) == "01210"_state);
static_assert(stateFromWord("pleat"_word, "trawl"_word) == "10101"_state);
static_assert(stateFromWord("shark"_word, "zanza"_word) == "01000"_state);
static_assert(stateFromWord("solar"_word, "abaca"_word) == "10000"_state);
static_assert(stateFromWord("solar"_word, "alaap"_word) == "01020"_state);
static_assert(stateFromWord("solar"_word, "alaap"_word) == "01020"_state);
static_assert(stateFromWord("solar"_word, "raise"_word) == "11010"_state);

static_assert(stateFromWord("basic"_word, "rayne"_word) == "02000"_state);
static_assert(stateFromWord("basic"_word, "humph"_word) == "00000"_state);

static_assert(stateFromWord("chute"_word, "rayne"_word) == "00002"_state);
static_assert(stateFromWord("chute"_word, "sluit"_word) == "00201"_state);

} // namespace wordle
