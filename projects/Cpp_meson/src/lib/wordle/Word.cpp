#include <wordle/Word.h>

#include <ostream>

namespace wordle {

std::ostream& operator<<(std::ostream& os, Word const& w) {
    for (auto ch : w) {
        os << static_cast<char>(ch + 'a');
    }
    return os;
}

} // namespace wordle
