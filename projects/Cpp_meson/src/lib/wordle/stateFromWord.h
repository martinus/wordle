#pragma once

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
    auto counts = std::array<uint8_t, 'z' - 'a' + 1>();

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

} // namespace wordle
