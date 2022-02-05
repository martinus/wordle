#pragma once

#include <wordle/Word.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace wordle {

enum class St : uint8_t { unspecified, not_included, wrong_spot, correct };

class State {
    std::array<St, NumCharacters> m_data{};

public:
    constexpr St& operator[](size_t idx) {
        return m_data[idx];
    }

    constexpr St const& operator[](size_t idx) const {
        return m_data[idx];
    }

    constexpr size_t size() const {
        return m_data.size();
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

    constexpr bool operator==(State const& other) const {
        for (size_t i = 0; i < NumCharacters; ++i) {
            if (m_data[i] != other[i]) {
                return false;
            }
        }
        return true;
    }
};

std::ostream& operator<<(std::ostream& os, State const& s);

} // namespace wordle
