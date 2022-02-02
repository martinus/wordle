#pragma once

#include <array>

namespace wordle {

// hardcoded constant - all words have these many characters
static constexpr auto NumCharacters = 5;

// Word stores byte 0-26 ('a' to 'z').
using Word = std::array<char, NumCharacters>;

} // namespace wordle
