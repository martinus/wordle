#pragma once

#include <wordle/Word.h>

#include <vector>
#include <iosfwd>

// Parses a dictionary, extracting words
namespace wordle {

std::vector<Word> parseDict(std::istream& in);

}