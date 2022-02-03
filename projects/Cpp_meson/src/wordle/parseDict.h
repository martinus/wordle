#pragma once

#include <wordle/Word.h>

#include <iosfwd>
#include <vector>

namespace wordle {

// Parses a dictionary, extracting words.
std::vector<Word> parseDict(std::istream& in);

} // namespace wordle
