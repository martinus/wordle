#include <wordle/parseDict.h>

#include <istream>
#include <unordered_set>
#include <vector>

namespace wordle {

namespace {

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

} // namespace

std::vector<Word> parseDict(std::istream& in) {
    auto word = std::string();
    auto words = std::vector<Word>();
    auto uniqueWords = std::unordered_set<std::string>();
    while (in >> word) {
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
        if (uniqueWords.emplace(word).second) {
            Word w{};
            for (size_t i = 0; i < word.size(); ++i) {
                w[i] = word[i] - 'a';
            }
            words.push_back(w);
        }
    }

    return words;
}

} // namespace wordle

namespace wordle::test {

static_assert(createUpperToLowercaseTable()['a'] == 'a');
static_assert(createUpperToLowercaseTable()['z'] == 'z');
static_assert(createUpperToLowercaseTable()['A'] == 'a');
static_assert(createUpperToLowercaseTable()['Z'] == 'z');
static_assert(createUpperToLowercaseTable()['1'] == 0);

} // namespace wordle::test
