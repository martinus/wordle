#include <util/parallel/for_each.h>
#include <wordle/AlphabetMap.h>
#include <wordle/IsWordValid.h>
#include <wordle/Word.h>
#include <wordle/parseDict.h>
#include <wordle/stateFromWord.h>

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
    /**
     * Maximum number of remaining words that will remain when using this guessing word. This is evaluated against all words
     * that can possibly be correct. This is the most important metric, because we want to optimize for the worst case to
     * guarantee that we can reduce the set of possible words as much as possible.
     */
    size_t maxCount = 0;

    /**
     * Quadratic error of the number of remaining words. The lower the better, but first we optimize
     * against the maximum. So when several words lead to the same maximum number of remaining words, we prefer the one where
     * the sum of count^2 is minimum.
     *
     * NOTE: My gut says this is a reasonable heuristic, but I have nothing to prove this.
     */
    size_t depth = 0;

    size_t numFilteredCorrectWords = 0;

    constexpr static Fitness maxi() {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
    }
    constexpr static Fitness mini() {
        return {};
    }

    /**
     * @brief Helper to make use of tuple's comparisons
     *
     * @return constexpr auto
     */
    constexpr auto asTuple() const {
        return std::tie(maxCount, depth, numFilteredCorrectWords);
    }

private:
    /**
     * Don't allow default ctor, only allow static worst() and best() functions so we know what we are getting
     */
    Fitness() = default;
};

std::ostream& operator<<(std::ostream& os, Fitness const& f) {
    return os << "(maxCount=" << f.maxCount << ", depth=" << f.depth
              << ", numFilteredCorrectWords=" << f.numFilteredCorrectWords << ")";
}

constexpr bool operator<=(Fitness const& a, Fitness const& b) {
    return a.asTuple() <= b.asTuple();
}
constexpr bool operator<(Fitness const& a, Fitness const& b) {
    return a.asTuple() < b.asTuple();
}
constexpr bool operator>=(Fitness const& a, Fitness const& b) {
    return a.asTuple() >= b.asTuple();
}
constexpr bool operator>(Fitness const& a, Fitness const& b) {
    return a.asTuple() > b.asTuple();
}
constexpr bool operator==(Fitness const& a, Fitness const& b) {
    return a.asTuple() == b.asTuple();
}
constexpr bool operator!=(Fitness const& a, Fitness const& b) {
    return a.asTuple() != b.asTuple();
}

/*
struct Results {
    Fitness fitness = Fitness::worst();
    std::vector<std::string_view> words{};
};
*/

struct Node {
    IsWordValid m_isWordValid;
    std::vector<Word>* m_allowedWordsToEnter;
    std::vector<Word>* m_remainingCorrectWords;
};

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

Result maxi(Node const& node, Word const& guessWord, size_t currentDepth, size_t maxDepth, Fitness alpha, Fitness beta);
Result mini(Node& node, size_t currentDepth, size_t maxDepth, Fitness alpha, Fitness beta);

// mini: wants to make a guess that lowers the number of remaining correct words as much as possible
Result mini(Node& node, size_t currentDepth, size_t maxDepth, Fitness alpha, Fitness beta) {
    if (node.m_remainingCorrectWords->size() == 1) {
        return Result{Fitness{0, currentDepth}, node.m_remainingCorrectWords->front()};
    }

    auto bestValue = Result::maxi();

    if (currentDepth == 0) {
        // depth 0: run loop in parallel
        auto mutex = std::mutex();
        ankerl::parallel::for_each(
            node.m_allowedWordsToEnter->begin(), node.m_allowedWordsToEnter->end(), [&](Word const& guessWord) {
                auto value = maxi(node, guessWord, currentDepth + 1, maxDepth, alpha, beta);

                auto lock = std::lock_guard(mutex);

                if (value.m_fitness < bestValue.m_fitness) {
                    bestValue.m_fitness = value.m_fitness;
                    bestValue.m_guessWord = guessWord;

                    std::cout << currentDepth << ": \"" << guessWord << "\" alpha=" << alpha.maxCount
                              << ", beta=" << beta.maxCount << ", fitness=" << value.m_fitness << std::endl;
                }

                if (bestValue.m_fitness <= alpha) {
                    // alpha cutoff, stop iterating
                    return ankerl::parallel::Continue::no;
                }
                beta = std::min(beta, bestValue.m_fitness);
                // continue iterating
                return ankerl::parallel::Continue::yes;
            });
    } else {
        for (Word const& guessWord : *node.m_allowedWordsToEnter) {
            auto value = maxi(node, guessWord, currentDepth + 1, maxDepth, alpha, beta);

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
Result maxi(Node const& node, Word const& guessWord, size_t currentDepth, size_t maxDepth, Fitness alpha, Fitness beta) {
    Result bestValue = Result::mini();
    auto maxFilteredCorrectWords = size_t();
    for (Word const& correctWord : *node.m_remainingCorrectWords) {
        // create information for the next node
        auto nextNode = node;
        auto state = stateFromWord(correctWord, guessWord);
        nextNode.m_isWordValid.addWordAndState(guessWord, state);

        // create a new list of remaining correct words
        auto value = Result();
        if (currentDepth == maxDepth - 1) {
            // we've reached the end, just calculate number of remaining words as the fitness value.
            value.m_fitness.maxCount = 0;
            value.m_fitness.depth = currentDepth;

            for (Word const& word : *node.m_remainingCorrectWords) {
                if (nextNode.m_isWordValid(word) && guessWord != word) {
                    ++value.m_fitness.maxCount;
                }
            }
        } else {
            // we have to go deeper
            auto filteredWords = std::vector<Word>();
            for (Word const& word : *node.m_remainingCorrectWords) {
                if (nextNode.m_isWordValid(word) && guessWord != word) {
                    filteredWords.push_back(word);
                }
            }
            nextNode.m_remainingCorrectWords = &filteredWords;
            value = mini(nextNode, currentDepth + 1, maxDepth, alpha, beta);
            maxFilteredCorrectWords = std::max(maxFilteredCorrectWords, filteredWords.size());
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
    bestValue.m_fitness.numFilteredCorrectWords = maxFilteredCorrectWords;
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

    auto isWordValid = wordle::IsWordValid();

    for (int i = 2; i < argc; ++i) {
        auto [word, state] = wordle::parseWordAndState(argv[i]);
        isWordValid.addWordAndState(word, state);
    }

    // pre.debugPrint();

    // create list of words that are currently valid
    auto filteredCorrectWords = std::vector<wordle::Word>();
    for (auto word : wordsCorrect) {
        if (isWordValid(word)) {
            filteredCorrectWords.push_back(word);
            std::cout << word << " ";
        }
    }
    std::cout << std::endl;

    auto node = wordle::Node{isWordValid, &allowedWords, &filteredCorrectWords};

    auto alpha = wordle::Fitness::mini();
    auto beta = wordle::Fitness::maxi();
    // auto beta = wordle::Fitness{2, 4};
    size_t currentDepth = 0;
    size_t maxDepth = 4;
    auto bestResult = wordle::alphabeta::mini(node, currentDepth, maxDepth, alpha, beta);

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
