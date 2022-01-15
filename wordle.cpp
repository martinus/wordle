#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

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

    // walks through all words and creates a list of ones that could match
    std::string validWords(std::string_view allWords) const {
        auto filtered = std::string();
        while (!allWords.empty()) {
            auto word = allWords.substr(0, NumCharacters);
            allWords = allWords.substr(NumCharacters);

            auto isWordAllowed = true;
            for (int i = 0; i < NumCharacters; ++i) {
                if (!m_allowedChars[i][word[i]]) {
                    isWordAllowed = false;
                    continue;
                }
            }
            if (!isWordAllowed) {
                continue;
            }

            // now check that each mandatory letter is actually used
            auto setLetters = std::array<bool, 256>{};
            auto numMandatoryLetters = 0;
            for (auto ch : word) {
                if (!setLetters[ch] && m_mandatoryChars[ch]) {
                    setLetters[ch] = true;
                    ++numMandatoryLetters;
                }
                setLetters[ch] = true;
            }
            if (numMandatoryLetters != m_numMandatoryLetters) {

                std::cout << word << " " << numMandatoryLetters << " " << m_numMandatoryLetters << std::endl;
                continue;
            }

            filtered += word;
        }
        return filtered;
    }
};

template <typename Op>
void eachWord(std::string_view words, Op&& op) {
    while (!words.empty()) {
        auto word = words.substr(0, NumCharacters);
        op(word);
        words = words.substr(NumCharacters);
    }
}

void showWords(std::string_view words) {
    eachWord(words, [](std::string_view word) {
        std::cout << word << std::endl;
    });
}

void evalWords(std::string_view words, Preconditions const& pre) {
    eachWord(words, [&](std::string_view winingWord) {
        eachWord(words, [&](std::string_view trialWord) {
            std::cout << winingWord << " " << trialWord << std::endl;
        });
    });
}

// Usage e.g. ./wordle weary00102 yelps10000 zones00200
//  0: letter doesnt exist
//  1: letter exists, but wrong position
//  2: letter is in correct spot!

// e.g. ./wordle dfhjkc .an.y
//  letters d, f, h, j, c are *not in the word.
//  letters an and y are in the right splot, the dot's are still missing.
//
// start with
//  ./wordle .....

int main(int argc, char** argv) {
    // read & filter dictionary
    auto words = readAndFilterDictionary("/usr/share/dict/american-english");

    auto pre = Preconditions(argc, argv);

    pre.debugPrint();

    auto filteredWords = pre.validWords(words);
    showWords(filteredWords);

    // evaluate words.
    // evalWords(filteredWords);
}
