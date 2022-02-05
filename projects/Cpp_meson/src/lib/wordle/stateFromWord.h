#pragma once

#include <wordle/AlphabetMap.h>
#include <wordle/State.h>
#include <wordle/Word.h>

#include <cstdint>

namespace wordle {

/**
 * @brief Given a correct word and a guessing word, calculates the color for each letter.
 *
 * Color is given as char array with numbers '0', '1', '2':
 *
 * '2': The letter is in the correct spot.
 * '1': The letter is in the word but in the wrong spot.
 * '0': The letter is not in the word in any spot.
 *
 * Note that there are a few special cases with repeated letters. E.g. for the correct word
 * "abcde" the guess "xaaxx" will result in "01000", so only the first 'a' gets a 1.
 *
 * This function is a bit tricky to get right, so beware! It has plenty of tests though.
 *
 * @param correctWord The correct word for which we are guessing.
 * @param guessWord A guessing word.
 * @return Char array with characters 0,1,2 to represent the matching state.
 */
constexpr State stateFromWord(Word const& correctWord, Word const& guessWord) {
    auto state = State();
    auto counts = AlphabetMap<uint8_t>();

    for (int i = 0; i < NumCharacters; ++i) {
        if (guessWord[i] == correctWord[i]) {
            state[i] = St::correct;
        } else {
            ++counts[correctWord[i]];
            state[i] = St::not_included;
        }
    }

    for (int i = 0; i < NumCharacters; ++i) {
        if (guessWord[i] != correctWord[i] && counts[guessWord[i]]) {
            state[i] = St::wrong_spot;
            --counts[guessWord[i]];
        }
    }

    return state;
}

class IsSingleWordValid {
    AlphabetMap<uint8_t> m_charAllowed{}; // maps from character to bitfield where it's allowed.
    AlphabetMap<uint8_t> m_mandatoryCharCount{};
    int m_numMandatoryCharCount{};

public:
    constexpr IsSingleWordValid(Word const& guessWord, State const& guessState)
        : m_charAllowed((1U << NumCharacters) - 1)
        , m_mandatoryCharCount()
        , m_numMandatoryCharCount(0) {
        // First, set all characters that are not allowed. Initially, everything is allowed everywhere.
        for (int i = 0; i < NumCharacters; ++i) {
            if (guessState[i] == St::not_included) {
                // not allowed anywhere
                m_charAllowed[guessWord[i]] = 0;
            }
        }

        // then, update all 1's: allow anywhere, except at the 1 position, and also not where letteris the same but state is 0
        for (int i = 0; i < NumCharacters; ++i) {
            if (guessState[i] == St::wrong_spot) {
                for (int j = 0; j < NumCharacters; ++j) {
                    if (guessWord[i] == guessWord[j]) {
                        // not allowed here
                        m_charAllowed[guessWord[i]] &= ~(1U << j);
                    } else {
                        // allowed here
                        m_charAllowed[guessWord[i]] |= (1U << j);
                    }
                }
                ++m_mandatoryCharCount[guessWord[i]];
                ++m_numMandatoryCharCount;
            }
        }

        // finally, update all 2's: allow exactly here,
        for (int i = 0; i < NumCharacters; ++i) {
            auto mask = ~(1U << i);
            if (guessState[i] == St::correct) {
                for (auto& x : m_charAllowed) {
                    x &= mask;
                }
                m_charAllowed[guessWord[i]] |= 1U << i;
                ++m_mandatoryCharCount[guessWord[i]];
                ++m_numMandatoryCharCount;
            }
        }
    }

    constexpr bool operator()(Word const& checkWord) const {
        for (int i = 0; i < NumCharacters; ++i) {
            if (0 == (m_charAllowed[checkWord[i]] & (1U << i))) {
                return false;
            }
        }

        auto mandatoryCharCount = m_mandatoryCharCount;
        auto numMandatoryCharCount = m_numMandatoryCharCount;
        for (int i = 0; i < NumCharacters; ++i) {
            if (mandatoryCharCount[checkWord[i]] != 0) {
                --mandatoryCharCount[checkWord[i]];
                --numMandatoryCharCount;
            }
        }

        return 0 == numMandatoryCharCount;
    }
};

} // namespace wordle
