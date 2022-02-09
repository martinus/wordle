#include <util/parallel/for_each.h>
#include <wordle/AlphabetMap.h>
#include <wordle/IsSingleWordValid.h>
#include <wordle/Word.h>
#include <wordle/parseDict.h>
#include <wordle/stateFromWord.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>

namespace wordle {

/**
 * @brief Fitness score of a guess word. The lower, the better.
 */
struct Fitness {
    // maximum number of remaining words for each level, but in reverse.
    std::array<size_t, 3> m_maxCounts{};

    constexpr size_t& operator[](size_t idx) {
        return m_maxCounts[m_maxCounts.size() - idx - 1];
    }

    constexpr static Fitness maxi() {
        auto f = Fitness();
        for (auto& x : f.m_maxCounts) {
            x = std::numeric_limits<size_t>::max();
        }
        return f;
    }
    constexpr static Fitness mini() {
        return {};
    }

private:
    /**
     * Don't allow default ctor, only allow static worst() and best() functions so we know what we are getting
     */
    Fitness() = default;
};

std::ostream& operator<<(std::ostream& os, Fitness const& f) {
    auto prefix = std::string_view("(");
    std::for_each(f.m_maxCounts.rbegin(), f.m_maxCounts.rend(), [&](auto const& x) {
        os << prefix << x;
        prefix = ", ";
    });
    return os << ")";
}

constexpr bool operator<=(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts <= b.m_maxCounts;
}
constexpr bool operator<(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts < b.m_maxCounts;
}
constexpr bool operator>=(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts >= b.m_maxCounts;
}
constexpr bool operator>(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts > b.m_maxCounts;
}
constexpr bool operator==(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts == b.m_maxCounts;
}
constexpr bool operator!=(Fitness const& a, Fitness const& b) {
    return a.m_maxCounts != b.m_maxCounts;
}

struct Result {
    Fitness m_fitness = Fitness::maxi();
    Word m_guessWord{};

    static Result maxi() {
        return {Fitness::maxi(), Word{}};
    }

    static Result mini() {
        return {Fitness::mini(), Word{}};
    }
};

enum class Player : bool { maxi, mini };

namespace alphabeta {

// see https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning#Pseudocode

Result maxi(std::vector<Word> const& allowedWordsToEnter,
            std::vector<Word> const& remainingCorrectWords,
            Word const& guessWord,
            size_t currentDepth,
            size_t maxDepth,
            Fitness alpha,
            Fitness beta);

Result mini(std::vector<Word> const& allowedWordsToEnter,
            std::vector<Word> const& remainingCorrectWords,
            size_t currentDepth,
            size_t maxDepth,
            Fitness alpha,
            Fitness beta);

// mini: wants to make a guess that lowers the number of remaining correct words as much as possible
Result mini(std::vector<Word> const& allowedWordsToEnter,
            std::vector<Word> const& remainingCorrectWords,
            size_t currentDepth,
            size_t maxDepth,
            Fitness alpha,
            Fitness beta) {
    if (remainingCorrectWords.size() == 1) {
        return Result{Fitness{0, currentDepth}, remainingCorrectWords.front()};
    }

    auto bestValue = Result::maxi();

    if (currentDepth == 0) {
        // depth 0: run loop in parallel
        auto mutex = std::mutex();
        ankerl::parallel::for_each(allowedWordsToEnter.begin(), allowedWordsToEnter.end(), [&](Word const& guessWord) {
            auto value = maxi(allowedWordsToEnter, remainingCorrectWords, guessWord, currentDepth, maxDepth, alpha, beta);

            auto lock = std::lock_guard(mutex);

            if (value.m_fitness < bestValue.m_fitness) {
                bestValue.m_fitness = value.m_fitness;
                bestValue.m_guessWord = guessWord;

                std::cout << currentDepth << ": \"" << guessWord << "\" alpha=" << alpha << ", beta=" << beta
                          << ", fitness=" << value.m_fitness << std::endl;
            }

            if (bestValue.m_fitness <= alpha) {
                // alpha cutoff, stop iterating
                return ankerl::parallel::Continue::no;
            }
            beta = std::min(beta, bestValue.m_fitness);
            // continue iterating
            return ankerl::parallel::Continue::yes;
        } /*, 1*/);
    } else {
        for (Word const& guessWord : allowedWordsToEnter) {
            auto value = maxi(allowedWordsToEnter, remainingCorrectWords, guessWord, currentDepth, maxDepth, alpha, beta);

            if (value.m_fitness < bestValue.m_fitness) {
                bestValue.m_fitness = value.m_fitness;
                bestValue.m_guessWord = guessWord;
            }

            if (bestValue.m_fitness <= alpha) {
                // alpha cutoff, stop iterating
                break;
            }
            beta = std::min(beta, bestValue.m_fitness);
        }
    }

    return bestValue;
}

// maxi: wants to find the most hard to guess "correct" word
Result maxi(std::vector<Word> const& allowedWordsToEnter,
            std::vector<Word> const& remainingCorrectWords,
            Word const& guessWord,
            size_t currentDepth,
            size_t maxDepth,
            Fitness alpha,
            Fitness beta) {
    Result bestValue = Result::mini();
    for (Word const& correctWord : remainingCorrectWords) {
        // create information for the next node
        auto state = stateFromWord(correctWord, guessWord);
        auto isWordValid = IsSingleWordValid(guessWord, state);

        // create a new list of remaining correct words
        auto value = Result();

        if (currentDepth == maxDepth - 1) {
            // we've reached the end, just calculate number of remaining words as the fitness value.
            value.m_fitness[currentDepth] = 0;

            for (Word const& word : remainingCorrectWords) {
                if (isWordValid(word) && guessWord != word) {
                    ++value.m_fitness[currentDepth];
                }
            }
        } else {
            // we have to go deeper
            auto filteredWords = std::vector<Word>();
            for (Word const& word : remainingCorrectWords) {
                if (isWordValid(word) && guessWord != word) {
                    filteredWords.push_back(word);
                }
            }
            value = mini(allowedWordsToEnter, filteredWords, currentDepth + 1, maxDepth, alpha, beta);
            value.m_fitness[currentDepth] = filteredWords.size();
        }

        if (value.m_fitness > bestValue.m_fitness) {
            bestValue = value;

            if (bestValue.m_fitness >= beta) {
                // beta cutoff, stop iterating
                break;
            }
            alpha = std::max(alpha, bestValue.m_fitness);
        }
    }
    return bestValue;
}

} // namespace alphabeta

std::pair<Word, State> parseWordAndState(std::string_view wordAndState) {
    if (wordAndState.size() != NumCharacters * 2) {
        throw std::runtime_error("incorrect number of letters");
    }

    auto word = Word();
    auto state = State();
    for (size_t i = 0; i < NumCharacters; ++i) {
        word[i] = wordAndState[i] - 'a';
        switch (wordAndState[i + NumCharacters]) {
        case '0':
            state[i] = St::not_included;
            break;
        case '1':
            state[i] = St::wrong_spot;
            break;
        case '2':
            state[i] = St::correct;
            break;
        default:
            throw std::runtime_error("invalid state character");
        }
    }
    return std::make_pair(word, state);
}

/**
 * @brief Reads & filters a dictionary file with newline separated words.
 *
 * Words are separated by '\n'. All are lowercase, exactly numCharacters long, sorted, and unique. No special characters.
 * Read in any dictionary file. This lowercases all words, and filters all out that don't have 5 characters or any special
 * character.
 *
 * @param filename Dictionary filename
 * @return std::string One string with all lowercase concatenated words (without any separator).
 */
std::vector<Word> readAndFilterDictionary(std::filesystem::path filename) {
    auto fin = std::ifstream(filename);
    if (!fin.is_open()) {
        throw std::runtime_error("Could not open " + filename.string());
    }
    return parseDict(fin);
}

// Calculates a score for each word, based on letter frequency
void heuristicSort(std::vector<Word>& words) {
    auto letterFrequency = AlphabetMap<size_t>();
    for (auto const& word : words) {
        for (auto ch : word) {
            ++letterFrequency[ch];
        }
    }

    auto scores = std::vector<std::pair<Word, size_t>>();
    for (auto const& word : words) {
        auto hasLetter = AlphabetMap<bool>();
        auto score = size_t();
        for (auto ch : word) {
            if (!hasLetter[ch]) {
                score += letterFrequency[ch];
                hasLetter[ch] = true;
            }
        }
        scores.emplace_back(word, score);
    }

    std::sort(scores.begin(), scores.end(), [](auto const& a, auto const& b) {
        return a.second < b.second;
    });

    for (size_t i = 0; i < scores.size(); ++i) {
        words[i] = scores[i].first;
    }
}

} // namespace wordle

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << R"(This is a wordle solver, written to assist in https://www.powerlanguage.co.uk/wordle/

Usage: ./wordle <prefix> [word-state]...

Examples:

    ./wordle dictionaries/en
        Loads dictionaries/en_allowed.txt and dictionaries/en_correct.txt, and calculates the best
        starting word(s). This can take a while!

    ./wordle dictionaries/en weary00102 yelps10000
        Loads english dictionaries, and give two guesses. Each guess consists of the word followed by
        the state for each letter, based on the color output of https://www.powerlanguage.co.uk/wordle/.

        0: letter does not exist (e.g. 'w', 'e', 'r' for weary00102)
        1: letter exists but wrong position (e.g. 'a' for weary00102)
        2: letter is in correct spot (e.g. 'y' for weary00102)

        Based on that input wordle gives the best word(s) to follow up, so the number of possibilities
        are reduced as much as possible.

by Martin Leitner-Ankerl 2022
)";

        exit(1);
    }
    // read & filter dictionary
    auto prefix = std::string(argv[1]);

    auto allowedWords = wordle::readAndFilterDictionary(prefix + "_allowed.txt");
    auto wordsCorrect = wordle::readAndFilterDictionary(prefix + "_correct.txt");

    wordle::heuristicSort(allowedWords);
    wordle::heuristicSort(wordsCorrect);
    std::reverse(allowedWords.begin(), allowedWords.end());

    auto validators = std::vector<wordle::IsSingleWordValid>();
    for (int i = 2; i < argc; ++i) {
        auto [word, state] = wordle::parseWordAndState(argv[i]);
        validators.emplace_back(word, state);
    }

    // pre.debugPrint();

    // create list of words that are currently valid
    auto filteredCorrectWords = std::vector<wordle::Word>();
    for (auto word : wordsCorrect) {
        auto it = std::find_if_not(validators.begin(), validators.end(), [&](auto const& validator) {
            return validator(word);
        });
        if (it == validators.end()) {
            filteredCorrectWords.push_back(word);
            std::cout << word << " ";
        }
    }
    std::cout << std::endl;

    auto alpha = wordle::Fitness::mini();
    auto beta = wordle::Fitness::maxi();
    size_t currentDepth = 0;
    size_t maxDepth = 2;

    auto bestResult = wordle::alphabeta::mini(allowedWords, filteredCorrectWords, currentDepth, maxDepth, alpha, beta);

    std::cout << bestResult.m_fitness << " " << bestResult.m_guessWord << std::endl;
}

/**
 * Tests evaluated at compile time
 */
namespace wordle::test {

constexpr Word toStateWord(std::string_view str) {
    Word w{};
    for (size_t i = 0; i < str.size(); ++i) {
        w[i] = str[i];
    }
    return w;
}

} // namespace wordle::test
