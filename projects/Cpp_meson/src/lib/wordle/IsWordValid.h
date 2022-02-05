#pragma once

#include <wordle/AlphabetMap.h>
#include <wordle/State.h>
#include <wordle/Word.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace wordle {

class IsWordValid {
    std::array<AlphabetMap<bool>, NumCharacters> m_allowedCharPerLetter{};
    AlphabetMap<uint8_t> m_mandatoryCharCount{};

    std::array<char, NumCharacters> m_mandatoryCharsForSearch{};
    size_t m_numMandatoryCharsForSearch{};

public:
    IsWordValid() {
        // initially, all letters are allowed
        for (char ch = 0; ch <= 'z' - 'a'; ++ch) {
            for (auto& ac : m_allowedCharPerLetter) {
                ac[ch] = true;
            }
        }
    }

    IsWordValid& addWordAndState(Word const& word, State const& state) {
#if 0
        std::cout << "addWordAndState: " << word << " " << state << ", mandatory='"
                  << toString(m_mandatoryCharsForSearch, m_numMandatoryCharsForSearch) << "'" << std::endl;
#endif
        auto newMandatoryChars = AlphabetMap<uint8_t>();
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            // correct=shark, guess=zanza, state=01000
            if (St::not_included == state[charIdx]) {
                if (newMandatoryChars[word[charIdx]]) {
                    // can only set this to false, earlier that character has already appeared as a 1, so it has to be
                    // somewhere else
                    m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                    // TODO also every else not allowed except for the '1'
                } else {
                    // only when that character is *not* anywhere else in the word,
                    // letter word[i] doesn't exist, not allowed at any place
                    for (auto& ac : m_allowedCharPerLetter) {
                        ac[word[charIdx]] = false;
                    }
                }
            } else if (St::wrong_spot == state[charIdx]) {
                // letter word[i] exists, but not at this place
                m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // do this *after* the other loops, so when 2 same are there and one matches and the other is not here, so that
        // this works
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if (St::correct == state[charIdx]) {
                // letter found! reset all letters
                m_allowedCharPerLetter[charIdx] = {};
                m_allowedCharPerLetter[charIdx][word[charIdx]] = true;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // now merge newMandatoryChars with the old ones, and build the string for search
        m_numMandatoryCharsForSearch = 0;
        for (char ch = 0; ch <= 'z' - 'a'; ++ch) {
            m_mandatoryCharCount[ch] = std::max(m_mandatoryCharCount[ch], newMandatoryChars[ch]);
            for (uint8_t i = 0; i < m_mandatoryCharCount[ch]; ++i) {
                m_mandatoryCharsForSearch[m_numMandatoryCharsForSearch] = ch;
                ++m_numMandatoryCharsForSearch;
            }
        }

        return *this;
    }

    /**
     * True if the given word is acceptable based on the current wordle state.
     *
     * Highly performance relevant!
     *
     * @param word The word to check
     */
    bool operator()(Word const& word) const {
        // check allowed characters
        for (int i = 0; i < NumCharacters; ++i) {
            if (!m_allowedCharPerLetter[i][word[i]]) {
                // word not allowed, continue with next word
                return false;
            }
        }

        // check that each mandatory letter is used
        auto w = word;

        for (size_t i = 0; i < m_numMandatoryCharsForSearch; ++i) {
            if (auto it = std::find(w.begin(), w.end(), m_mandatoryCharsForSearch[i]); it != w.end()) {
                // set to an invalid char so it won't be can't be found again, in case of the same 2 letters
                *it = 127;
            } else {
                // mandatory char not present, continue with next word
                return false;
            }
        }
        return true;
    }
};

} // namespace wordle