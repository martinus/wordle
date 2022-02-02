#include <parallel/for_each.h>
#include <wordle/Word.h>

#include <algorithm>
#include <array>
#include <cstring> // memcpy
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace wordle {

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
constexpr Word stateFromWord(Word const& correctWord, Word const& guessWord) {
    auto state = Word();
    auto counts = std::array<uint8_t, 'z' - 'a' + 1>();

    for (int i = 0; i < NumCharacters; ++i) {
        if (guessWord[i] == correctWord[i]) {
            // got the correct letter!
            state[i] = '2';
        } else {
            ++counts[correctWord[i]];
            state[i] = '0';
        }
    }

    for (int i = 0; i < NumCharacters; ++i) {
        if (guessWord[i] != correctWord[i] && counts[guessWord[i]]) {
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
std::vector<Word> readAndFilterDictionary(std::filesystem::path filename) {
    auto fin = std::ifstream(filename);
    if (!fin.is_open()) {
        throw std::runtime_error("Could not open " + filename.string());
    }
    auto word = std::string();

    // std::set so it's unique and sorted
    auto words = std::vector<Word>();
    auto uniqueWords = std::unordered_set<std::string>();
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

/**
 * @brief Collection of words, all with the same number of letters.
 */
class Words {
    std::vector<Word> m_words{};

public:
    Words(std::vector<Word>&& words)
        : m_words(std::move(words)) {}

    auto begin() const {
        return m_words.begin();
    }

    auto end() const {
        return m_words.end();
    }

    auto begin() {
        return m_words.begin();
    }

    auto end() {
        return m_words.end();
    }

    /**
     * @brief Number of words
     */
    size_t size() const {
        return m_words.size();
    }

    bool empty() const {
        return m_words.empty();
    }

    std::vector<Word> const& words() const {
        return m_words;
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
        return m_data[ch];
    }

    constexpr Mapped const& operator[](char ch) const {
        // no out of bounds check because this has to be very fast
        return m_data[ch];
    }
};

// for each spot check which letters are still allowed
// create set of characters that need to be there

/**
 * Based on several words & states,
 *
 */
class Preconditions {
    std::array<AlphabetMap<bool>, NumCharacters> m_allowedCharPerLetter{};
    AlphabetMap<uint8_t> m_mandatoryCharCount{};

    // 0-terminated, therefore n+1!
    std::array<char, NumCharacters + 1> m_mandatoryCharsForSearch{};

public:
    Preconditions() {
        // initially, all letters are allowed
        for (char ch = 0; ch <= 'z' - 'a'; ++ch) {
            for (auto& ac : m_allowedCharPerLetter) {
                ac[ch] = true;
            }
        }
    }

    void addWordAndState(std::string_view wordAndState) {
        if (wordAndState.size() != NumCharacters * 2) {
            throw std::runtime_error("incorrect number of letters");
        }

        Word word{};
        Word state{};
        for (size_t i = 0; i < NumCharacters; ++i) {
            word[i] = wordAndState[i] - 'a';
            state[i] = wordAndState[i + NumCharacters];
        }
        addWordAndState(word, state);
    }

    void addWordAndState(Word const& word, Word const& state) {
        // std::cout << "addWordAndState: " << word << " " << state << ", mandatory=" << m_mandatoryCharsForSearch;
        auto newMandatoryChars = AlphabetMap<uint8_t>();
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            // correct=shark, guess=zanza, state=01000
            if ('0' == state[charIdx]) {
                if (newMandatoryChars[word[charIdx]]) {
                    // can only set this to false, earlier that character has already appeared as a 1, so it has to be
                    // somewhere else
                    m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                    // TODO also every else not allowed except for the '1'
                } else {
                    // only when that character is *not* anywhere else in the word,
                    // letter word[i] doesn't exist, not allowed at any place
                    for (auto& ac : m_allowedCharPerLetter) {
                        ac[word[charIdx]] = false;
                    }
                }
            } else if ('1' == state[charIdx]) {
                // letter word[i] exists, but not at this place
                m_allowedCharPerLetter[charIdx][word[charIdx]] = false;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // do this *after* the other loops, so when 2 same are there and one matches and the other is not here, so that
        // this works
        for (size_t charIdx = 0; charIdx < NumCharacters; ++charIdx) {
            if ('2' == state[charIdx]) {
                // letter found! reset all letters
                m_allowedCharPerLetter[charIdx] = {};
                m_allowedCharPerLetter[charIdx][word[charIdx]] = true;
                ++newMandatoryChars[word[charIdx]];
            }
        }

        // now merge newMandatoryChars with the old ones, and build the string for search
        m_mandatoryCharsForSearch = {};
        auto* mandatoryCharsForSearchPtr = m_mandatoryCharsForSearch.data();
        for (char ch = 0; ch <= 'z' - 'a'; ++ch) {
            m_mandatoryCharCount[ch] = std::max(m_mandatoryCharCount[ch], newMandatoryChars[ch]);
            for (uint8_t i = 0; i < m_mandatoryCharCount[ch]; ++i) {
                *mandatoryCharsForSearchPtr = ch;
                ++mandatoryCharsForSearchPtr;
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
        std::cout << " " << m_mandatoryCharsForSearch.data() << std::endl;
    }

    /**
     * True if the given word is acceptable based on the current wordle state.
     *
     * Highly performance relevant!
     *
     * @param word The word to check
     */
    bool isWordValid(Word const& word) const {
        // check allowed characters
        for (int i = 0; i < NumCharacters; ++i) {
            if (!m_allowedCharPerLetter[i][word[i]]) {
                // word not allowed, continue with next word
                return false;
            }
        }

        // check that each mandatory letter is used
        std::array<char, NumCharacters> w{};
        std::copy(word.data(), word.data() + NumCharacters, w.data());

        auto const* mandatoryPtr = m_mandatoryCharsForSearch.data();
        while (0 != *mandatoryPtr) {
            if (auto it = std::find(w.begin(), w.end(), *mandatoryPtr); it != w.end()) {
                // set to 0 so it won't be can't be found again, in case of the same 2 letters
                *it = 0;
            } else {
                // mandatory char not present, continue with next word
                return false;
            }
            ++mandatoryPtr;
        }
        return true;
    }

    /**
     * @brief Walks through all words and creates a list of ones that could match.
     *
     * This is highly performance critical!
     */
    template <typename Op>
    constexpr bool eachValidWord(Words const& allWords, Op&& op) const {
        for (Word const& word : allWords) {
            if (!isWordValid(word)) {
                // not valid, continue with next word
                continue;
            }

            // convenience: when op returns bool use this, otherwise just call it
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
};

std::string toString(Word const& word) {
    std::string str;
    for (auto ch : word) {
        str += ch + 'a';
    }
    return str;
}

void showWords(Words const& words) {
    auto prefix = "";
    for (Word const& word : words) {
        std::cout << prefix << toString(word);
        prefix = " ";
    }
    std::cout << std::endl;
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
    size_t depth = 0;

    constexpr static Fitness maxi() {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
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
        return std::tie(maxCount, depth);
    }

private:
    /**
     * Don't allow default ctor, only allow static worst() and best() functions so we know what we are getting
     */
    Fitness() = default;
};

std::ostream& operator<<(std::ostream& os, Fitness const& f) {
    return os << "(maxCount=" << f.maxCount << ", depth=" << f.depth << ")";
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
    Preconditions m_pre;
    Words* m_allowedWordsToEnter;
    Words* m_remainingCorrectWords;
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

template <typename ElementIterator>
class IterateMultiple {
    using ContainerIterators = std::vector<std::pair<ElementIterator, ElementIterator>>;
    ContainerIterators m_beginEndIters{};

public:
    class It {
        ElementIterator m_elementIter{};
        typename ContainerIterators::const_iterator m_containerIter{};

    public:
        constexpr It(ElementIterator elementIter, typename ContainerIterators::const_iterator containerIter)
            : m_elementIter(elementIter)
            , m_containerIter(containerIter) {}

        constexpr auto operator*() const {
            return *m_elementIter;
        }

        constexpr It& operator++() {
            if (++m_elementIter == m_containerIter->second) {
                ++m_containerIter;
                m_elementIter = m_containerIter->first;
            }
            return *this;
        }

        constexpr bool operator==(It const& other) const {
            return m_containerIter == other.m_containerIter && m_elementIter == other.m_elementIter;
        }

        constexpr bool operator!=(It const& other) const {
            return !(*this == other);
        }
    };

    IterateMultiple() {
        // add sentinel
        m_beginEndIters.emplace_back();
    }

    IterateMultiple& add(ElementIterator begin, ElementIterator end) {
        m_beginEndIters.pop_back();
        m_beginEndIters.emplace_back(begin, end);
        m_beginEndIters.emplace_back();
        return *this;
    }

    It begin() const {
        return {m_beginEndIters.front().first, m_beginEndIters.begin()};
    }

    It end() const {
        auto backIt = m_beginEndIters.end();
        return {{}, --backIt};
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
        return Result{Fitness{0, currentDepth}, node.m_remainingCorrectWords->words().front()};
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

                    std::cout << currentDepth << ": \"" << toString(guessWord) << "\" alpha=" << alpha.maxCount
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
    for (Word const& correctWord : *node.m_remainingCorrectWords) {
        // create information for the next node
        auto nextNode = node;
        auto state = stateFromWord(correctWord, guessWord);
        nextNode.m_pre.addWordAndState(guessWord, state);

        // create a new list of remaining correct words
        auto value = Result();
        if (currentDepth == maxDepth - 1) {
            // we've reached the end, just calculate number of remaining words as the fitness value.
            value.m_fitness.maxCount = 0;
            value.m_fitness.depth = currentDepth;

            for (Word const& word : *node.m_remainingCorrectWords) {
                if (nextNode.m_pre.isWordValid(word) && guessWord != word) {
                    ++value.m_fitness.maxCount;
                }
            }
        } else {
            // we have to go deeper
            auto filteredWordsStr = std::vector<Word>();
            for (Word const& word : *node.m_remainingCorrectWords) {
                if (nextNode.m_pre.isWordValid(word) && guessWord != word) {
                    filteredWordsStr.push_back(word);
                }
            }
            auto filteredCorrectWords = Words(std::move(filteredWordsStr));
            nextNode.m_remainingCorrectWords = &filteredCorrectWords;
            value = mini(nextNode, currentDepth + 1, maxDepth, alpha, beta);
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

    auto allowedWords = wordle::Words(wordle::readAndFilterDictionary(prefix + "_allowed.txt"));
    auto wordsCorrect = wordle::Words(wordle::readAndFilterDictionary(prefix + "_correct.txt"));

    auto pre = wordle::Preconditions();

    for (int i = 2; i < argc; ++i) {
        pre.addWordAndState(argv[i]);
    }

    // pre.debugPrint();

    // create list of words that are currently valid
    auto filteredWordsStr = std::vector<wordle::Word>();
    for (auto word : wordsCorrect) {
        if (pre.isWordValid(word)) {
            filteredWordsStr.push_back(word);
            std::cout << wordle::toString(word) << " ";
        }
    }
    std::cout << std::endl;
    auto filteredCorrectWords = wordle::Words(std::move(filteredWordsStr));

    auto node = wordle::Node{pre, &allowedWords, &filteredCorrectWords};

    auto alpha = wordle::Fitness::mini();
    auto beta = wordle::Fitness::maxi();
    // auto beta = wordle::Fitness{2, 4};
    size_t currentDepth = 0;
    size_t maxDepth = 4;
    auto bestResult = wordle::alphabeta::mini(node, currentDepth, maxDepth, alpha, beta);

    std::cout << bestResult.m_fitness << " " << wordle::toString(bestResult.m_guessWord) << std::endl;
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

constexpr Word stateFromWord(std::string_view correctWord, std::string_view guessWord) {
    Word wa{};
    Word wb{};
    for (size_t i = 0; i < NumCharacters; ++i) {
        wa[i] = correctWord[i] - 'a';
        wb[i] = guessWord[i] - 'a';
    }
    return wordle::stateFromWord(wa, wb);
}

constexpr bool operator==(Word const& a, Word const& b) {
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

constexpr Word toStateWord(std::string_view str) {
    Word w{};
    for (size_t i = 0; i < str.size(); ++i) {
        w[i] = str[i];
    }
    return w;
}

static_assert(stateFromWord("aacde", "aaaxx") == toStateWord("22000"));
static_assert(stateFromWord("knoll", "pills") == toStateWord("00120"));
static_assert(stateFromWord("aacde", "aaxxx") == toStateWord("22000"));
static_assert(stateFromWord("abcde", "aaxxx") == toStateWord("20000"));
static_assert(stateFromWord("abcde", "xaaxx") == toStateWord("01000"));
static_assert(stateFromWord("gouge", "bough") == toStateWord("02220"));
static_assert(stateFromWord("gouge", "lento") == toStateWord("01001"));
static_assert(stateFromWord("gouge", "raise") == toStateWord("00002"));
static_assert(stateFromWord("jeans", "ashen") == toStateWord("11011"));
static_assert(stateFromWord("jeans", "knelt") == toStateWord("01100"));
static_assert(stateFromWord("jeans", "raise") == toStateWord("01011"));
static_assert(stateFromWord("lilac", "apian") == toStateWord("00120"));
static_assert(stateFromWord("lilac", "mambo") == toStateWord("01000"));
static_assert(stateFromWord("lilac", "stare") == toStateWord("00100"));
static_assert(stateFromWord("panic", "chase") == toStateWord("10100"));
static_assert(stateFromWord("panic", "chase") == toStateWord("10100"));
static_assert(stateFromWord("panic", "magic") == toStateWord("02022"));
static_assert(stateFromWord("panic", "rocky") == toStateWord("00100"));
static_assert(stateFromWord("shark", "zanza") == toStateWord("01000"));
static_assert(stateFromWord("solar", "abaca") == toStateWord("10000"));
static_assert(stateFromWord("solar", "alaap") == toStateWord("01020"));
static_assert(stateFromWord("solar", "alaap") == toStateWord("01020"));
static_assert(stateFromWord("solar", "raise") == toStateWord("11010"));

} // namespace wordle::test
