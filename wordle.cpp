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

static constexpr auto NumCharacters = 5;

// map from A..Z -> a..z
constexpr auto createUpperToLowercaseTable() -> std::array<char, 256> {
    auto data = std::array<char, 256>{}; // 0-initialize
    for (auto ch = 'a'; ch != 'z' + 1; ++ch) {
        data[ch] = ch;
        data[ch + 'A' - 'a'] = ch;
    }
    return data;
}

namespace testCreateUpperToLowercaseTable {

constexpr auto map = createUpperToLowercaseTable();
static_assert(map['a'] == 'a');
static_assert(map['z'] == 'z');
static_assert(map['A'] == 'a');
static_assert(map['Z'] == 'z');
static_assert(map['1'] == 0);

} // namespace testCreateUpperToLowercaseTable

// words are separated by '\n'. All are lowercase, exactly numCharacters long, sorted, and unique. No special characters.
std::string readAndFilterDictionary(std::filesystem::path filename) {
    auto fin = std::ifstream(filename);
    auto words = std::string();
    auto word = std::string();

    auto uniqueWords = std::set<std::string>(); // set so it's sorted
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
        if (counts[guessWord[i]]) {
            state[i] = '1';
            --counts[guessWord[i]];
        }
    }

    return state;
}

template <typename T, size_t N>
constexpr bool eq(std::array<T, N> const& a, std::array<T, N> const& b) {
    for (size_t i = 0; i < N; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

static_assert(eq(stateFromWord("panic", "chase"), std::array{'1', '0', '1', '0', '0'}));
static_assert(eq(stateFromWord("panic", "rocky"), std::array{'0', '0', '1', '0', '0'}));
static_assert(eq(stateFromWord("panic", "magic"), std::array{'0', '2', '0', '2', '2'}));
static_assert(eq(stateFromWord("abcde", "xaaxx"), std::array{'0', '1', '0', '0', '0'}));
static_assert(eq(stateFromWord("abcde", "aaxxx"), std::array{'2', '0', '0', '0', '0'}));
static_assert(eq(stateFromWord("aacde", "aaxxx"), std::array{'2', '2', '0', '0', '0'}));
static_assert(eq(stateFromWord("aacde", "aaaxx"), std::array{'2', '2', '0', '0', '0'}));
static_assert(eq(stateFromWord("shark", "zanza"), std::array{'0', '1', '0', '0', '0'}));
static_assert(eq(stateFromWord("solar", "raise"), std::array{'1', '1', '0', '1', '0'}));
static_assert(eq(stateFromWord("solar", "abaca"), std::array{'1', '0', '0', '0', '0'}));
static_assert(eq(stateFromWord("solar", "alaap"), std::array{'0', '1', '0', '2', '0'}));
static_assert(eq(stateFromWord("solar", "alaap"), std::array{'0', '1', '0', '2', '0'}));

static_assert(eq(stateFromWord("lilac", "mambo"), std::array{'0', '1', '0', '0', '0'}));
static_assert(eq(stateFromWord("lilac", "stare"), std::array{'0', '0', '1', '0', '0'}));
static_assert(eq(stateFromWord("lilac", "apian"), std::array{'0', '0', '1', '2', '0'}));

template <typename Op>
constexpr bool eachWord(std::string_view words, Op&& op) {
    while (!words.empty()) {
        auto word = words.substr(0, NumCharacters);
        if (!op(word)) {
            return false;
        }
        words = words.substr(NumCharacters);
    }
    return true;
}

// for each spot check which letters are still allowed
// create set of characters that need to be there
class Preconditions {
    std::array<std::array<bool, 256>, NumCharacters> m_allowedChars{};
    std::array<uint8_t, 256> m_mandatoryCharCount{};
    std::string m_mandatoryCharsForSearch{};

public:
    Preconditions(int argc, char** argv) {
        // initially, all letters are allowed
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            for (auto& ac : m_allowedChars) {
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
                    m_allowedChars[charIdx][word[charIdx]] = false;
                } else {
                    // only when that caracter is *not* anywhere else in the word,
                    // letter word[i] doesn't exist, not allowed at any place
                    for (auto& ac : m_allowedChars) {
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
                m_allowedChars[charIdx][word[charIdx]] = false;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // do this *after* the other loops, so when 2 same are there and one matches and the other is not here, so that
        // this works
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('2' == state[charIdx]) {
                // letter found!
                m_allowedChars[charIdx] = {};
                m_allowedChars[charIdx][word[charIdx]] = true;
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
                if (m_allowedChars[i][ch]) {
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

    std::string validWords(std::string_view allWords) const {
        auto filtered = std::string();
        eachValidWord(allWords, [&](std::string_view validWord) -> bool {
            filtered += validWord;
            return true;
        });
        return filtered;
    }

    // walks through all words and creates a list of ones that could match
    template <typename Op>
    bool eachValidWord(std::string_view allWords, Op&& op) const {
        return eachWord(allWords, [&](std::string_view word) -> bool {
            // check allowed characters
            for (int i = 0; i < NumCharacters; ++i) {
                if (!m_allowedChars[i][word[i]]) {
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

void showWords(std::string_view words) {
    auto prefix = "";
    eachWord(words, [&](std::string_view word) -> bool {
        std::cout << prefix << word;
        prefix = " ";
        return true;
    });
    std::cout << std::endl;
}

#if 0
// For each word to guess, calculate how well it can filter down the list of remaining words.
// The more a guessing word reduces the number of correct words, the better.
void evalWords(std::string_view wordsAllowed, std::string_view filteredWords, Preconditions const& pre) {
    auto fitnessBestWord = std::numeric_limits<size_t>::max();

    eachWord(wordsAllowed, [&](std::string_view guessWord) -> bool {
        auto fitnessGuessWord = size_t();

        eachWord(filteredWords, [&](std::string_view correctWord) -> bool {
            if (correctWord == guessWord) {
                return true;
            }

            auto preCopy = pre;
            auto state = stateFromWord(correctWord, guessWord);
            preCopy.addWordAndState(guessWord, std::string_view(state.data(), state.size()));

            // count number of possible matches - the less the better
            auto sum = size_t();
            auto ret = preCopy.eachValidWord(filteredWords, [&](std::string_view word) -> bool {
                ++sum;
                return fitnessGuessWord + sum * sum <= fitnessBestWord;
            });
            fitnessGuessWord += sum * sum;
            return ret;
        });

        if (fitnessGuessWord <= fitnessBestWord) {
            std::cout << fitnessGuessWord << " " << guessWord << std::endl;
            fitnessBestWord = fitnessGuessWord;
        }
        return true;
    });
}

#else

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

Results evalWords(std::string_view wordsAllowed, std::string_view filteredWords, Preconditions const& pre) {
    auto results = Results();
    results.fitness = Fitness::worst();

    eachWord(wordsAllowed, [&](std::string_view guessWord) -> bool {
        auto fitnessGuessWord = Fitness{0, 0, WordState::trial};
        eachWord(filteredWords, [&](std::string_view correctWord) -> bool {
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
#endif

// Usage e.g. ./wordle en weary00102 yelps10000 zones00200
//  0: letter doesnt exist
//  1: letter exists, but wrong position
//  2: letter is in correct spot!

int main(int argc, char** argv) {
    // read & filter dictionary
    auto language = std::string(argv[1]);

    auto wordsAllowed = readAndFilterDictionary("words_" + language + "_allowed.txt");
    auto wordsCorrect = readAndFilterDictionary("words_" + language + "_correct.txt");

    auto pre = Preconditions(argc - 1, argv + 1);

    // pre.debugPrint();

    auto filteredCorrectWords = pre.validWords(wordsCorrect);
    // showWords(filteredWords);

    // evaluate words.
    auto numPotentialWords = size_t();
    eachWord(filteredCorrectWords, [&](std::string_view word) -> bool {
        ++numPotentialWords;
        return true;
    });

    auto results = evalWords(wordsAllowed, filteredCorrectWords, pre);

    if (numPotentialWords == 1) {
        std::cout << "The correct word is \"" << results.words.front() << "\"." << std::endl;
    } else {
        // showWords(filteredCorrectWords);
        std::cout << numPotentialWords << " possible words. "
                  << (results.fitness.wordState == WordState::maybe ? "Try " : "Limit down with ")
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
