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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "useful.hxx"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <stdexcept>

namespace msc
{

    namespace /*anonymous*/
    {

        int deref_it(const char *const s)
        {
            return static_cast<int>(static_cast<unsigned char>(*s));
        }

        bool one_or_more_decimal_digits(const std::string_view text) noexcept
        {
            if (text.empty()) {
                return false;
            }
            return std::all_of(std::begin(text), std::end(text), [](const unsigned char c){ return std::isdigit(c); });
        }

    }  // namespace /*anonymous*/

    std::string normalize_constant_name(const std::string_view name)
    {
        auto result = std::string{};
        if (!name.empty()) {
            const auto s = name.data();
            std::size_t i = 0;
            std::size_t j = name.size();
            while ((i < j) && std::isspace(deref_it(s + i))) ++i;
            while ((j > i) && std::isspace(deref_it(s + j - 1))) --j;
            result.reserve(j - i);
            std::transform(
                s + i, s + j, std::back_inserter(result),
                [](const unsigned char c){ return (c == '_') ? '-' : std::tolower(c); }
            );
        }
        return result;
    }

    [[noreturn]] void reject_invalid_enumeration(const int value, const std::string_view name)
    {
        // We build the message this way rather than using our own `concat` function because we can and because it helps
        // us avoid creating cyclic references between our components.
        const auto message = std::to_string(value)
            .append(" is not a valid constant for an enumerator of type '")
            .append(name.data(), name.size())
            .append("'");
        throw std::invalid_argument{message};
    }

    [[noreturn]] void reject_invalid_enumeration(const std::string_view value, const std::string_view name)
    {
        // We build the message this way rather than using our own `concat` function because we can and because it helps
        // us avoid creating cyclic references between our components.
        const auto message = std::string{"'"}
            .append(value.data(), value.size())
            .append("' is not a valid name for an enumerator of type '")
            .append(name.data(), name.size())
            .append("'");
        throw std::invalid_argument{message};
    }

    std::optional<int> parse_decimal_number(std::string_view text) noexcept
    {
        // Our definition of "looks like a decimal integer" is much stricter than what `strtol` would parse so we have
        // to check beforehand but after that, we can rest assured that `strtol` will succeed unless there was an
        // overflow which is also handled by the (again possibly stricter) comparison against `INT_MAX`.
        constexpr auto length = std::size_t{32};
        if ((text.size() <= length) && one_or_more_decimal_digits(text)) {
            char buffer[length + 1];
            std::memcpy(buffer, text.data(), text.size());
            buffer[text.size()] = '\0';
            const auto value = std::strtol(buffer, nullptr, 10);
            if (value <= INT_MAX) {
                return static_cast<int>(value);
            }
        }
        return std::nullopt;
    }

}  // namespace msc
