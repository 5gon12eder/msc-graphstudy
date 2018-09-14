// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2018 Karlsruhe Institute of Technology
// Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.

/**
 * @file strings.hxx
 *
 * @brief
 *     Common string utilities.
 *
 */

#ifndef MSC_STRINGS_HXX
#define MSC_STRINGS_HXX

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

namespace msc
{

    /**
     * @brief
     *     Concatenates zero or more string-like objects.
     *
     * @tparam Ts...
     *     types of the objects to concatenate (`char*`, `std::string`, `std::string_view`, ...)
     *
     * @param parts
     *     string-like objects to concatenate
     *
     * @returns
     *     concatenated string
     *
     */
    template <typename... Ts>
    std::enable_if_t<std::conjunction_v<std::is_convertible<Ts, std::string_view>...>, std::string>
    concat(Ts&&... parts)
    {
        const std::array<std::string_view, sizeof...(Ts)> viewed_parts = {{ std::forward<Ts>(parts)... }};
        auto total_length = std::size_t{};
        for (auto&& part : viewed_parts) {
            total_length += part.length();
        }
        auto result = std::string{};
        result.reserve(total_length);
        for (auto&& part : viewed_parts) {
            result.append(part.data(), part.length());
        }
        return result;
    }

    /**
     * @brief
     *     Tests whether `prefix` is a prefix of `text`.
     *
     * @param text
     *     text to test
     *
     * @param prefix
     *     prefix to test
     *
     * @returns
     *     whether `prefix` is a prefix of `text`
     *
     */
    inline bool startswith(const std::string_view text, const std::string_view prefix) noexcept
    {
        if (text.length() < prefix.length()) {
            return false;
        }
        return (0 == text.compare(0, prefix.length(), prefix));
    }

    /**
     * @brief
     *     Tests whether `suffix` is a suffix of `text`.
     *
     * @param text
     *     text to test
     *
     * @param suffix
     *     suffix to test
     *
     * @returns
     *     whether `suffix` is a suffix of `text`
     *
     */
    inline bool endswith(const std::string_view text, const std::string_view suffix) noexcept
    {
        if (text.length() < suffix.length()) {
            return false;
        }
        return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
    }

}  // namespace msc

#endif  // !defined(MSC_STRINGS_HXX)
