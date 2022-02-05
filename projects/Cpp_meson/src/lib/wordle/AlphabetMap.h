#pragma once

#include <array>
#include <cstddef>

namespace wordle {

/**
 * @brief Fast map from alphabet 'a'..'z' to a value
 *
 * Since the number of possible values is very small (and fixed), this can be optimized well.
 */
template <typename Mapped>
class AlphabetMap {
    std::array<Mapped, 'z' - 'a' + 1> m_data{};

public:
    constexpr AlphabetMap(Mapped val) {
        // m_data.fill(val); // fill is not constexpr :-(
        for (auto i = size_t(); i < m_data.size(); ++i) {
            m_data[i] = val;
        }
    }

    constexpr AlphabetMap()
        : m_data{} {}

    constexpr Mapped& operator[](char ch) {
        // no out of bounds check because this has to be very fast
        return m_data[ch];
    }

    constexpr Mapped const& operator[](char ch) const {
        // no out of bounds check because this has to be very fast
        return m_data[ch];
    }

    constexpr auto begin() {
        return m_data.begin();
    }

    constexpr auto end() {
        return m_data.end();
    }
};

} // namespace wordle
