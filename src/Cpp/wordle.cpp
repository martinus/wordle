#include <algorithm>
#include <array>
#include <cstring> // memcpy
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <string>

namespace wordle {

// hardcoded constant - all words have these many characters
static constexpr auto NumCharacters = 5;

/**
 * @brief Uppercase to lowercase conversion map A..Z -> a..z.
 *
 * Invalid characters are mapped to 0.
 */
constexpr auto createUpperToLowercaseTable() -> std::array<char, 256> {
    auto data = std::array<char, 256>{}; // 0-initialize
    for (auto ch = 'a'; ch != 'z' + 1; ++ch) {
        data[ch] = ch;
        data[ch + 'A' - 'a'] = ch;
    }
    return data;
}

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
constexpr std::array<char, NumCharacters> stateFromWord(std::string_view correctWord, std::string_view guessWord) {
    auto state = std::array<char, NumCharacters>();
    auto counts = std::array<uint8_t, 256>();
    for (auto ch : correctWord) {
        ++counts[ch];
    }

    for (int i = 0; i < NumCharacters; ++i) {
        state[i] = '0';
        if (guessWord[i] == correctWord[i]) {
            // got the correct letter!
            state[i] = '2';
            --counts[guessWord[i]];
        }
    }

    for (int i = 0; i < NumCharacters; ++i) {
        if (counts[guessWord[i]] && state[i] != '2') {
            state[i] = '1';
            --counts[guessWord[i]];
        }
    }

    return state;
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
std::string readAndFilterDictionary(std::filesystem::path filename) {
    auto fin = std::ifstream(filename);
    if (!fin.is_open()) {
        throw std::runtime_error("Could not open " + filename.string());
    }
    auto words = std::string();
    auto word = std::string();

    // std::set so it's unique and sorted
    auto uniqueWords = std::set<std::string>();
    while (fin >> word) {
        if (word.size() != NumCharacters) {
            continue;
        }

        bool allLettersValid = true;
        for (auto& ch : word) {
            constexpr auto map = createUpperToLowercaseTable();
            ch = map[ch];
            if (ch == 0) {
                allLettersValid = false;
                continue;
            }
        }

        if (!allLettersValid) {
            continue;
        }
        uniqueWords.emplace(std::move(word));
    }

    for (auto const& word : uniqueWords) {
        words += word;
    }

    return words;
}

/**
 * @brief Collection of words, all with the same number of letters.
 *
 * TODO don't use a global constant for word length
 */
class Words {
    // All words, without separator
    std::string m_words{};

public:
    Words(std::string&& words)
        : m_words(std::move(words)) {}

    /**
     * @brief Iterates all words and calls op with each word.
     *
     * Stops iterating when op returns true. For convenience, op can also be a void() function.
     * @return True when op() always returned true (or when op is void), otherwise false.
     */
    template <typename Op>
    constexpr bool each(Op&& op) const {
        for (size_t idx = 0; idx < m_words.size(); idx += NumCharacters) {
            auto word = std::string_view(m_words.data() + idx, NumCharacters);

            // when op returns a bool, use a false result to bail out.
            if constexpr (std::is_same_v<void, std::invoke_result_t<Op, std::string_view>>) {
                op(word);
            } else {
                if (!op(word)) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief Number of words
     */
    size_t size() const {
        return m_words.size() / NumCharacters;
    }
};

/**
 * @brief Fast map from alphabet 'a'..'z' to a value
 *
 * Since the number of possible values is very small (and fixed), this can be optimized well.
 */
template <typename Mapped>
class AlphabetMap {
    std::array<Mapped, 'z' - 'a' + 1> m_data{};

public:
    constexpr Mapped& operator[](char ch) {
        // no out of bounds check because this has to be very fast
        return m_data[ch - 'a'];
    }

    constexpr Mapped const& operator[](char ch) const {
        // no out of bounds check because this has to be very fast
        return m_data[ch - 'a'];
    }
};

// for each spot check which letters are still allowed
// create set of characters that need to be there

class Preconditions {
    std::array<AlphabetMap<bool>, NumCharacters> m_allowedCharPerLetter{};
    AlphabetMap<uint8_t> m_mandatoryCharCount{};
    std::string m_mandatoryCharsForSearch{};

public:
    Preconditions() {
        // initially, all letters are allowed
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            for (auto& ac : m_allowedCharPerLetter) {
                ac[ch] = true;
            }
        }
    }

    void addWordAndState(std::string_view wordAndState) {
        if (wordAndState.size() != NumCharacters * 2) {
            throw std::runtime_error("incorrect number of letters");
        }

        auto word = std::string_view(wordAndState.data(), NumCharacters);
        auto state = std::string_view(wordAndState.data() + NumCharacters, NumCharacters);
        addWordAndState(word, state);
    }

    void addWordAndState(std::string_view word, std::string_view state) {
        // std::cout << "addWordAndState: " << word << " " << state << ", mandatory=" << m_mandatoryCharsForSearch;
        auto earlierOne = AlphabetMap<bool>();
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            // correct=shark, guess=zanza, state=01000
            if ('0' == state[charIdx]) {
                if (earlierOne[word[charIdx]]) {
                    // can only set this to false, an earlier of that char could be somewhere else
                    m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                } else {
                    // only when that character is *not* anywhere else in the word,
                    // letter word[i] doesn't exist, not allowed at any place
                    for (auto& ac : m_allowedCharPerLetter) {
                        ac[word[charIdx]] = false;
                    }
                }
            } else if ('1' == state[charIdx]) {
                earlierOne[word[charIdx]] = true;
            }
        }

        auto newMandatoryChars = AlphabetMap<uint8_t>();
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('1' == state[charIdx]) {
                // letter word[i] exists, but not at this place
                m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // do this *after* the other loops, so when 2 same are there and one matches and the other is not here, so that
        // this works
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('2' == state[charIdx]) {
                // letter found!
                m_allowedCharPerLetter[charIdx] = {};
                m_allowedCharPerLetter[charIdx][word[charIdx]] = true;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // now merge newMandatoryChars with the old ones, and build the string for search
        m_mandatoryCharsForSearch.clear();
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            m_mandatoryCharCount[ch] = std::max(m_mandatoryCharCount[ch], newMandatoryChars[ch]);
            for (uint8_t i = 0; i < m_mandatoryCharCount[ch]; ++i) {
                m_mandatoryCharsForSearch += ch;
            }
        }
        // std::cout << " mandatory after=" << m_mandatoryCharsForSearch << std::endl;
    }

    void debugPrint() const {
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            std::cout << ch;
        }
        std::cout << std::endl;
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            std::cout << '-';
        }
        std::cout << std::endl;

        for (int i = 0; i < NumCharacters; ++i) {
            for (char ch = 'a'; ch <= 'z'; ++ch) {
                if (m_allowedCharPerLetter[i][ch]) {
                    std::cout << ch;
                } else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
        }
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            std::cout << '-';
        }
        std::cout << " " << m_mandatoryCharsForSearch << std::endl;
    }

    Words validWords(Words const& allWords) const {
        auto filtered = std::string();
        eachValidWord(allWords, [&](std::string_view validWord) -> bool {
            filtered += validWord;
            return true;
        });
        return Words(std::move(filtered));
    }

    /**
     * @brief Walks through all words and creates a list of ones that could match.
     *
     * This is highly performance critical!
     */
    template <typename Op>
    bool eachValidWord(Words const& allWords, Op&& op) const {
        return allWords.each([&](std::string_view word) -> bool {
            // check allowed characters
            for (int i = 0; i < NumCharacters; ++i) {
                if (!m_allowedCharPerLetter[i][word[i]]) {
                    // word not allowed, continue with next word
                    return true;
                }
            }

            // check that each mandatory letter is used
            std::array<char, NumCharacters> w;
            std::memcpy(w.data(), word.data(), NumCharacters);

            for (auto mandatoryChar : m_mandatoryCharsForSearch) {
                if (auto it = std::find(w.begin(), w.end(), mandatoryChar); it != w.end()) {
                    // set to 0 so it won't be can't be found again, in case of the same 2 letters
                    *it = 0;
                } else {
                    // mandatory char not present, continue with next word
                    return true;
                }
            }

            // convenience: when op returns bool use this, otherwise just call it
            if constexpr (std::is_same_v<void, std::invoke_result_t<Op, std::string_view>>) {
                op(word);
                return true;
            } else {
                return op(word);
            }
        });
    }
};

void showWords(Words const& words) {
    auto prefix = "";
    words.each([&](std::string_view word) {
        std::cout << prefix << word;
        prefix = " ";
    });
    std::cout << std::endl;
}

enum WordState { partOfCorrectWords, notPartOfCorrectWords };

constexpr std::string_view toString(WordState ws) {
    switch (ws) {
    case WordState::partOfCorrectWords:
        return "part of correct words";
    case WordState::notPartOfCorrectWords:
        return "not part of correct words";
    }
    return "<unknown>";
}

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
    size_t sumCountSquared = 0;

    /**
     * Words can be either trial words (that are guaranteed not to be the correct words). Even though these words can be used
     * to limit down the number of possibilities, they certainly won't match. So all other things being equal, we prefer words
     * that are in the set of potential winners.
     */
    WordState wordState = WordState::notPartOfCorrectWords;

    constexpr static Fitness worst() {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), WordState::notPartOfCorrectWords};
    }
    constexpr static Fitness best() {
        return {};
    }

    /**
     * @brief Helper to make use of tuple's comparisons
     *
     * @return constexpr auto
     */
    constexpr auto asTuple() const {
        return std::tie(maxCount, sumCountSquared, wordState);
    }

private:
    /**
     * Don't allow default ctor, only allow static worst() and best() functions so we know what we are getting
     */
    Fitness() = default;
};

constexpr bool operator<=(Fitness const& a, Fitness const& b) {
    return a.asTuple() <= b.asTuple();
}
constexpr bool operator>=(Fitness const& a, Fitness const& b) {
    return a.asTuple() >= b.asTuple();
}
constexpr bool operator==(Fitness const& a, Fitness const& b) {
    return a.asTuple() == b.asTuple();
}
constexpr bool operator!=(Fitness const& a, Fitness const& b) {
    return a.asTuple() != b.asTuple();
}

struct Results {
    Fitness fitness = Fitness::worst();
    std::vector<std::string_view> words{};
};

Results evalWords(Words const& allowedWords, Words const& filteredWords, Preconditions const& pre) {
    auto results = Results();

    allowedWords.each([&](std::string_view guessWord) -> bool {
        auto fitnessGuessWord = Fitness{0, 0, WordState::notPartOfCorrectWords};
        filteredWords.each([&](std::string_view correctWord) -> bool {
            if (correctWord == guessWord) {
                fitnessGuessWord.wordState = WordState::partOfCorrectWords;
                return true;
            }

            auto preCopy = pre;
            auto state = stateFromWord(correctWord, guessWord);
            preCopy.addWordAndState(guessWord, std::string_view(state.data(), state.size()));

            // count number of possible matches - the less the better
            auto count = size_t();
            preCopy.eachValidWord(filteredWords, [&](std::string_view validWord) {
                ++count;
            });

            fitnessGuessWord.maxCount = std::max(fitnessGuessWord.maxCount, count);
            fitnessGuessWord.sumCountSquared += count * count;

            // as an optimization, stop evaluating filtered words when the fitness is worse than the best fitness we already
            // have. For debugging, use "return true" to evaluate all words.
            return fitnessGuessWord <= results.fitness;
        });

        if (fitnessGuessWord <= results.fitness) {
            if (fitnessGuessWord != results.fitness) {
                // got a new lower fitness value
                results.words.clear();
                results.fitness = fitnessGuessWord;
            }
            results.words.push_back(guessWord);
        }
        return true;
    });
    return results;
}

} // namespace wordle

// Usage e.g. ./wordle en weary00102 yelps10000 zones00200
//  0: letter doesnt exist
//  1: letter exists, but wrong position
//  2: letter is in correct spot!

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

        0: letter doesnt exist (e.g. 'w', 'e', 'r' for weary00102)
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

    auto allowedWords = wordle::Words(wordle::readAndFilterDictionary(prefix + "_allowed.txt"));
    auto wordsCorrect = wordle::Words(wordle::readAndFilterDictionary(prefix + "_correct.txt"));

    auto pre = wordle::Preconditions();

    for (int i = 2; i < argc; ++i) {
        pre.addWordAndState(argv[i]);
    }

    // pre.debugPrint();

    auto filteredCorrectWords = pre.validWords(wordsCorrect);
    // showWords(filteredWords);

    // evaluate words.
    auto numPotentialWords = filteredCorrectWords.size();

    auto results = wordle::evalWords(allowedWords, filteredCorrectWords, pre);

    if (numPotentialWords == 1) {
        std::cout << "The correct word is \"" << results.words.front() << "\"." << std::endl;
    } else {
        // showWords(filteredCorrectWords);
        std::cout << numPotentialWords << " possible words. "
                  << (results.fitness.wordState == wordle::WordState::partOfCorrectWords ? "Try " : "Limit down with ")
                  << (results.words.size() == 1 ? "this word" : "one of these") << " to rule out at least "
                  << (numPotentialWords - results.fitness.maxCount) << " ("
                  << (100.0 * (numPotentialWords - results.fitness.maxCount) / numPotentialWords) << "%, "
                  << results.fitness.maxCount << " remain) of these:";
        auto prefix = " ";
        for (auto const& word : results.words) {
            std::cout << prefix << '"' << word << '"';
            prefix = ", ";
        }

        std::cout << std::endl;
    }

    // std::cout << "wordState=" << toString(results.fitness.wordState) << std::endl;
}

/**
 * Tests evaluated at compile time
 */
namespace wordle::test {

static_assert(createUpperToLowercaseTable()['a'] == 'a');
static_assert(createUpperToLowercaseTable()['z'] == 'z');
static_assert(createUpperToLowercaseTable()['A'] == 'a');
static_assert(createUpperToLowercaseTable()['Z'] == 'z');
static_assert(createUpperToLowercaseTable()['1'] == 0);

/**
 * Creates an std::string_view from an std::array, e.g. for use in comparisons
 */
template <size_t N>
constexpr std::string_view toStringView(std::array<char, N> const& ary) {
    return std::string_view(ary.data(), ary.size());
}

static_assert(toStringView(stateFromWord("aacde", "aaaxx")) == "22000");
static_assert(toStringView(stateFromWord("aacde", "aaxxx")) == "22000");
static_assert(toStringView(stateFromWord("abcde", "aaxxx")) == "20000");
static_assert(toStringView(stateFromWord("abcde", "xaaxx")) == "01000");
static_assert(toStringView(stateFromWord("gouge", "bough")) == "02220");
static_assert(toStringView(stateFromWord("gouge", "lento")) == "01001");
static_assert(toStringView(stateFromWord("gouge", "raise")) == "00002");
static_assert(toStringView(stateFromWord("jeans", "ashen")) == "11011");
static_assert(toStringView(stateFromWord("jeans", "knelt")) == "01100");
static_assert(toStringView(stateFromWord("jeans", "raise")) == "01011");
static_assert(toStringView(stateFromWord("lilac", "apian")) == "00120");
static_assert(toStringView(stateFromWord("lilac", "mambo")) == "01000");
static_assert(toStringView(stateFromWord("lilac", "stare")) == "00100");
static_assert(toStringView(stateFromWord("panic", "chase")) == "10100");
static_assert(toStringView(stateFromWord("panic", "chase")) == "10100");
static_assert(toStringView(stateFromWord("panic", "magic")) == "02022");
static_assert(toStringView(stateFromWord("panic", "rocky")) == "00100");
static_assert(toStringView(stateFromWord("shark", "zanza")) == "01000");
static_assert(toStringView(stateFromWord("solar", "abaca")) == "10000");
static_assert(toStringView(stateFromWord("solar", "alaap")) == "01020");
static_assert(toStringView(stateFromWord("solar", "alaap")) == "01020");
static_assert(toStringView(stateFromWord("solar", "raise")) == "11010");

} // namespace wordle::test
