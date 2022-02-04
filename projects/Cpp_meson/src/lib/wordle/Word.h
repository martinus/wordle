#pragma once

#include <array>
#include <iosfwd>

namespace wordle {

// hardcoded constant - all words have these many characters
static constexpr auto NumCharacters = 5;

// Word stores byte 0-26 ('a' to 'z').
class Word {
    std::array<char, NumCharacters> m_data{};

public:
    constexpr char& operator[](size_t idx) {
        return m_data[idx];
    }

    constexpr char const& operator[](size_t idx) const {
        return m_data[idx];
    }

    constexpr size_t size() const {
        return m_data.size();
    }

    constexpr char const* data() const {
        return m_data.data();
    }

    constexpr auto begin() const {
        return m_data.begin();
    }

    constexpr auto begin() {
        return m_data.begin();
    }

    constexpr auto end() const {
        return m_data.end();
    }

    constexpr auto end() {
        return m_data.end();
    }

    constexpr bool operator==(Word const& other) const {
        // can't use std::array's == because it's not constexpr
        for (size_t i = 0; i < size(); ++i) {
            if (m_data[i] != other.m_data[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(Word const& other) const {
        return !(*this == other);
    }
};

std::ostream& operator<<(std::ostream& os, Word const& w);

} // namespace wordle
