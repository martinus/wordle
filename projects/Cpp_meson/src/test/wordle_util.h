#pragma once

#include <wordle/State.h>
#include <wordle/Word.h>

#include <stdexcept>

namespace wordle {

constexpr Word operator""_word(char const* str, size_t s) {
    auto w = Word();
    if (s != w.size()) {
        throw std::runtime_error("size does not match");
    }
    for (size_t i = 0; i < w.size(); ++i) {
        w[i] = str[i] - 'a';
    }
    return w;
}

constexpr State operator""_state(char const* str, size_t s) {
    auto st = State();
    if (s != st.size()) {
        throw std::runtime_error("size does not match");
    }

    for (size_t i = 0; i < st.size(); ++i) {
        switch (str[i]) {
        case '2':
            st[i] = St::correct;
            break;

        case '1':
            st[i] = St::wrong_spot;
            break;

        case '0':
            st[i] = St::not_included;
            break;
        default:
            throw std::runtime_error("wrong char");
        }
    }
    return st;
}

} // namespace wordle
