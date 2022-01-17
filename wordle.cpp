#include <algorithm> // find
#include <array>
#include <cmath>
#include <cstring> // memcpy
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    auto words = std::string();
    auto word = std::string();

    // std::set so it's unique and sorted
    auto uniqueWords = std::set<std::string>();
    while (std::getline(fin, word)) {
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
            if constexpr (std::is_same_v<bool, std::invoke_result_t<Op, std::string_view>>) {
                if (!op(word)) {
                    return false;
                }
            } else {
                op(word);
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
 * Since the number of possible values is very small (and fixed, this can be optimized well.
 *
 */
template <typename Mapped>
class AlphabetMap {
    std::array<Mapped, 256> m_data{};

public:
    constexpr Mapped& operator[](char ch) {
        return m_data[ch];
    }

    constexpr Mapped const& operator[](char ch) const {
        return m_data[ch];
    }
};

// for each spot check which letters are still allowed
// create set of characters that need to be there

class Preconditions {
    std::array<AlphabetMap<bool>, NumCharacters> m_allowedCharPerLetter{};
    AlphabetMap<uint8_t> m_mandatoryCharCount{};
    std::string m_mandatoryCharsForSearch{};

public:
    Preconditions(int argc, char** argv) {
        // initially, all letters are allowed
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            for (auto& ac : m_allowedCharPerLetter) {
                ac[ch] = true;
            }
        }

        for (int i = 1; i < argc; ++i) {
            addWordAndState(argv[i]);
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
        std::array<bool, 256> earlierOne{};
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            // correct=shark, guess=zanza, state=01000
            if ('0' == state[charIdx]) {
                if (earlierOne[word[charIdx]]) {
                    // can only set this to false, an earlier of that char could be somewhere else
                    m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                } else {
                    // only when that caracter is *not* anywhere else in the word,
                    // letter word[i] doesn't exist, not allowed at any place
                    for (auto& ac : m_allowedCharPerLetter) {
                        ac[word[charIdx]] = false;
                    }
                }
            } else if ('1' == state[charIdx]) {
                earlierOne[word[charIdx]] = true;
            }
        }

        auto newMandatoryChars = std::array<uint8_t, 256>{};
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

    // walks through all words and creates a list of ones that could match
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
                    *it = '_';
                } else {
                    // mandatory char not present, continue with next word
                    return true;
                }
            }

            return op(word);
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

enum WordState { maybe, trial };

constexpr std::string_view toString(WordState ws) {
    switch (ws) {
    case WordState::maybe:
        return "maybe";
    case WordState::trial:
        return "trial";
    }
    return "<unknown>";
}

struct Fitness {
    size_t maxCount = 0;
    size_t sum = 0;
    WordState wordState = WordState::trial;

    constexpr static Fitness worst() {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), WordState::trial};
    }
    constexpr static Fitness best() {
        return {0, 0, WordState::maybe};
    }
};

constexpr bool operator<=(Fitness const& a, Fitness const& b) {
    if (a.maxCount != b.maxCount) {
        return a.maxCount <= b.maxCount;
    }
    if (a.sum != b.sum) {
        return a.sum <= b.sum;
    }
    return a.wordState <= b.wordState;
}

constexpr bool operator>=(Fitness const& a, Fitness const& b) {
    if (a.maxCount != b.maxCount) {
        return a.maxCount >= b.maxCount;
    }
    if (a.sum != b.sum) {
        return a.sum >= b.sum;
    }
    return a.wordState >= b.wordState;
}

constexpr bool operator==(Fitness const& a, Fitness const& b) {
    return a.maxCount == b.maxCount && a.sum == b.sum && a.wordState == b.wordState;
}

constexpr bool operator!=(Fitness const& a, Fitness const& b) {
    return !(a == b);
}

struct Results {
    Fitness fitness{};
    std::vector<std::string_view> words{};
};

Results evalWords(Words const& wordsAllowed, Words const& filteredWords, Preconditions const& pre) {
    auto results = Results();
    results.fitness = Fitness::worst();

    wordsAllowed.each([&](std::string_view guessWord) -> bool {
        auto fitnessGuessWord = Fitness{0, 0, WordState::trial};
        filteredWords.each([&](std::string_view correctWord) -> bool {
            if (correctWord == guessWord) {
                fitnessGuessWord.wordState = WordState::maybe;
                return true;
            }

            auto preCopy = pre;
            auto state = stateFromWord(correctWord, guessWord);
            preCopy.addWordAndState(guessWord, std::string_view(state.data(), state.size()));

            // count number of possible matches - the less the better
            auto count = size_t();
            auto ret = preCopy.eachValidWord(filteredWords, [&](std::string_view validWord) -> bool {
                // std::cout << "guess=" << guessWord << ", correct=" << correctWord << ", valid=" << validWord << std::endl;
                ++count;
                return true;
            });
            // std::cout << "guessWord=" << guessWord << ", count=" << count << std::endl;
            fitnessGuessWord.maxCount = std::max(fitnessGuessWord.maxCount, count);
            fitnessGuessWord.sum += count * count;
            return fitnessGuessWord <= results.fitness;
            // return true;
        });
        // showWords(filteredWords);
        /*
                std::cout << fitnessGuessWord.maxCount << " " << fitnessGuessWord.sum << " " <<
           toString(fitnessGuessWord.wordState)
                          << ": " << guessWord << std::endl;

        */
        if (fitnessGuessWord <= results.fitness) {
            if (fitnessGuessWord != results.fitness) {
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
    // read & filter dictionary
    auto language = std::string(argv[1]);

    auto wordsAllowed = wordle::Words(wordle::readAndFilterDictionary("words_" + language + "_allowed.txt"));
    auto wordsCorrect = wordle::Words(wordle::readAndFilterDictionary("words_" + language + "_correct.txt"));

    auto pre = wordle::Preconditions(argc - 1, argv + 1);

    // pre.debugPrint();

    auto filteredCorrectWords = pre.validWords(wordsCorrect);
    // showWords(filteredWords);

    // evaluate words.
    auto numPotentialWords = filteredCorrectWords.size();

    auto results = wordle::evalWords(wordsAllowed, filteredCorrectWords, pre);

    if (numPotentialWords == 1) {
        std::cout << "The correct word is \"" << results.words.front() << "\"." << std::endl;
    } else {
        // showWords(filteredCorrectWords);
        std::cout << numPotentialWords << " possible words. "
                  << (results.fitness.wordState == wordle::WordState::maybe ? "Try " : "Limit down with ")
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
