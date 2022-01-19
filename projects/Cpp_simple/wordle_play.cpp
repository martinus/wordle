#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static constexpr auto NumCharacters = 5;

std::vector<std::string> readWords(std::filesystem::path const& filename) {
    auto fin = std::ifstream(filename);
    auto word = std::string();
    auto words = std::vector<std::string>();
    while (std::getline(fin, word)) {
        words.push_back(word);
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
        if (counts[guessWord[i]] && state[i] != '2') {
            state[i] = '1';
            --counts[guessWord[i]];
        }
    }

    return state;
}

// clang++ -O3 -std=c++17 wordle_play.cpp -o play
int main(int argc, char** argv) {
    auto prefix = std::string(argv[1]);
    auto correctWord = std::string();
    if (argc == 3) {
        correctWord = argv[2];
    } else {
        auto wordsAllowed = readWords(prefix + "_correct.txt");
        // select a random word
        auto rng = std::random_device{};
        auto dist = std::uniform_int_distribution<size_t>(0, wordsAllowed.size() - 1);
        correctWord = wordsAllowed[dist(rng)];
    }

    auto guessWord = std::string();
    std::cout << "guess #1: ";
    int guessNr = 2;

    while (std::cin >> guessWord) {
        if (guessWord == "?") {
            std::cout << "Correct word: " << correctWord << std::endl;
        }

        auto state = stateFromWord(correctWord, guessWord);
        if (guessWord == correctWord) {
            std::cout << "CORRECT!" << std::endl;
            return 0;
        }
        std::cout << std::string_view(state.data(), state.size()) << std::endl;

        std::cout << "\nguess #" << guessNr << ": ";
        ++guessNr;
    }
}