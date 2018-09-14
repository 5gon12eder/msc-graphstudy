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

#ifndef MSC_INCLUDED_FROM_NUMERIC_HXX
#  error "Never `#include <numeric.txx>` directly, `#include <numeric.hxx>` instead"
#endif

#include <cassert>
#include <iterator>

namespace msc
{

    namespace detail::numeric
    {

    }  // namespace detail::numeric

    template <typename FwdIterT>
    double integrate_trapezoidal(const FwdIterT first, const FwdIterT last) noexcept
    {
        assert(first != last);
        auto result = 0.0;
        auto prev = first;
        for (auto it = std::next(first); it != last; ++it) {
            const auto [x1, y1] = *prev;
            const auto [x2, y2] = *it;
            assert(x1 <= x2);
            result += (x2 - x1) * (y1 + y2) / 2.0;
            prev = it;
        }
        return result;
    }

}  // namespace msc
