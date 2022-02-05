#pragma once

#include <array>

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
    constexpr Mapped& operator[](char ch) {
        // no out of bounds check because this has to be very fast
        return m_data[ch];
    }

    constexpr Mapped const& operator[](char ch) const {
        // no out of bounds check because this has to be very fast
        return m_data[ch];
    }
};

} // namespace wordle
