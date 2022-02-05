#include <wordle/State.h>

#include <ostream>

namespace wordle {

std::ostream& operator<<(std::ostream& os, State const& s) {
    for (auto st : s) {
        auto ch = '!';
        switch (st) {
        case St::correct:
            ch = '2';
            break;

        case St::not_included:
            ch = '0';
            break;

        case St::wrong_spot:
            ch = '1';
            break;

        case St::unspecified:
            ch = '?';
        }
        os << ch;
    }
    return os;
}

} // namespace wordle
