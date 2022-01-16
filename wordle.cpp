#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>

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
    for (int i = 0; i < NumCharacters; ++i) {
        state[i] = '0';
        if (guessWord[i] == correctWord[i]) {
            // got the correct letter!
            state[i] = '2';
        } else if (std::string_view::npos != correctWord.find(guessWord[i])) {
            state[i] = '1';
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
    std::array<bool, 256> m_mandatoryChars{};
    int m_numMandatoryLetters{};

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
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('0' == state[charIdx]) {
                // letter word[i] doesn't exist, not allowed at any place
                for (auto& ac : m_allowedChars) {
                    ac[word[charIdx]] = false;
                }
            }
        }

        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('1' == state[charIdx]) {
                // letter word[i] exists, but not at this place
                m_allowedChars[charIdx][word[charIdx]] = false;
                m_mandatoryChars[word[charIdx]] = true;
            }
        }

        // do this *after* the other loops, so when 2 same are there and one matches and the other is not here, so that
        // this works
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('2' == state[charIdx]) {
                // letter found!
                m_allowedChars[charIdx] = {};
                m_allowedChars[charIdx][word[charIdx]] = true;
                // no need to set mandatory chars here, this is already checked
                // m_mandatoryChars[ch] = true;
            }
        }

        // update mandatory letters count
        m_numMandatoryLetters = 0;
        for (auto m : m_mandatoryChars) {
            if (m) {
                ++m_numMandatoryLetters;
            }
        }
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
        std::cout << std::endl;
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            if (m_mandatoryChars[ch]) {
                std::cout << ch;
            } else {
                std::cout << "#";
            }
        }
        std::cout << " " << m_numMandatoryLetters << std::endl;
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
            for (int i = 0; i < NumCharacters; ++i) {
                if (!m_allowedChars[i][word[i]]) {
                    // word not allowed, continue with next word
                    return true;
                }
            }

            // now check that each mandatory letter is actually used
            auto setLetters = std::array<bool, 'z' - 'a' + 1>{};
            auto numMandatoryLetters = 0;
            for (auto ch : word) {
                auto idx = ch - 'a';
                if (!setLetters[idx] && m_mandatoryChars[ch]) {
                    setLetters[idx] = true;
                    ++numMandatoryLetters;
                }
            }
            if (numMandatoryLetters != m_numMandatoryLetters) {
                // std::cout << word << " " << numMandatoryLetters << " " << m_numMandatoryLetters << std::endl;
                return true;
            }

            return op(word);
        });
    }
};

void showWords(std::string_view words) {
    eachWord(words, [](std::string_view word) -> bool {
        std::cout << word << std::endl;
        return true;
    });
}

#if 1
// Calculates a word's fitness:
// For each word,
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
            auto count = size_t();
            auto ret = preCopy.eachValidWord(filteredWords, [&](std::string_view word) -> bool {
                ++count;
                return true;
            });
            fitnessGuessWord = std::max(fitnessGuessWord, count);
            return fitnessGuessWord <= fitnessBestWord;
        });

        if (fitnessGuessWord <= fitnessBestWord) {
            std::cout << fitnessGuessWord << " " << guessWord << std::endl;
            fitnessBestWord = fitnessGuessWord;
        }
        return true;
    });
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
    evalWords(wordsAllowed, filteredCorrectWords, pre);
}
