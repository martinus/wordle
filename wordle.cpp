#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

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
std::string readAndFilterDictionary(std::filesystem::path filename, size_t numCharacters) {
    auto fin = std::ifstream(filename);
    auto words = std::string();
    auto word = std::string();

    auto uniqueWords = std::set<std::string>(); // set so it's sorted
    while (std::getline(fin, word)) {
        if (word.size() != numCharacters) {
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
        words += '\n';
    }

    return words;
}

int main(int argc, char** argv) {
    // read & filter dictionary
    auto words = readAndFilterDictionary("/usr/share/dict/american-english", 5);

    std::cout << words << std::endl;
}
