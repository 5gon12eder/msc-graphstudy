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

#ifndef MSC_INCLUDED_FROM_EDGE_CROSSING_HXX
#  error "Never `#include <edge_crossing.txx>` directly, `#include <edge_crossing.hxx>` instead"
#endif

#include <functional>
#include <tuple>

namespace msc
{

    namespace detail::edge_crossing
    {

        template <typename T, typename... Ts, typename CompT>
        constexpr auto my_minval([[maybe_unused]] CompT comp, const T& head, const Ts&... tail) noexcept -> T
        {
            if constexpr (sizeof...(Ts) == 0) {
                return head;
            } else {
                const auto skull = my_minval(comp, tail...);
                return comp(skull, head) ? skull : head;
            }
        }

        template <typename T>
        constexpr auto check_intersect_colinear(const planar_line<T> l1, const planar_line<T> l2) noexcept
            -> std::optional<point<T, 2>>
        {
            constexpr auto lexcmp = point_order<T>{};
            const auto [ll, sl, lr, sr] = [l1, l2](){
                const auto center = (l1.first + l1.second + l2.first + l2.second) / T{4};
                const auto lexmin = my_minval(lexcmp, l1.first, l1.second, l2.first, l2.second);
                const auto univec = lexmin - center;
                const auto s1 = std::make_pair(dot(univec, l1.first - center), dot(univec, l1.second - center));
                const auto s2 = std::make_pair(dot(univec, l2.first - center), dot(univec, l2.second - center));
                const auto s1x = (s1.second < s1.first) ? std::make_pair(s1.second, s1.first) : s1;
                const auto s2x = (s2.second < s2.first) ? std::make_pair(s2.second, s2.first) : s2;
                const auto l1x = (s1.second < s1.first) ? std::make_pair(l1.second, l1.first) : l1;
                const auto l2x = (s2.second < s2.first) ? std::make_pair(l2.second, l2.first) : l2;
                return (s2x < s1x) ? std::make_tuple(l2x, s2x, l1x, s1x) : std::make_tuple(l1x, s1x, l2x, s2x);
            }();
            if      (sl.second < sr.first)   return std::nullopt;                   // A------B  C---D
            else if (sl.second > sr.second)  return (lr.first + lr.second) / T{2};  // A---C--D---B
            else                             return (lr.first + ll.second) / T{2};  // A---C--B---------D
        }

    }  // namespace detail::edge_crossing

    template <typename T>
    constexpr std::optional<point<T, 2>> check_intersect(const planar_line<T> l1, const planar_line<T> l2) noexcept
    {
        constexpr auto zero = T{0};
        constexpr auto one = T{1};
        const auto c1 = l1.first - l1.second;
        const auto c2 = l2.second - l2.first;
        const auto c3 = l2.second - l1.second;
        const auto det = c1.x() * c2.y() - c2.x() * c1.y();
        if (det == zero) {
            const auto subdet1 = c3.x() * c2.y() - c2.x() * c3.y();
            const auto subdet2 = c1.x() * c3.y() - c3.x() * c1.y();
            if ((subdet1 == zero) && (subdet2 == zero)) {
                // Line segments are colinear; an infinite number of intersections /might/ exist.
                // Transform the problem into a one-dimensional coordinate system in a way that is deterministic in the
                // sense that order and direction of the line segments does not affect the result.
                return detail::edge_crossing::check_intersect_colinear(l1, l2);
            }
            // Line segments are parallel but not colinear; no intersection can exist.
            return std::nullopt;
        }
        // Line segments are not parallel; at most one intersection may exist.
        const T inverse[2][2] = {
            {+c2.y() / det, -c2.x() / det},
            {-c1.y() / det, +c1.x() / det},
        };
        const auto alpha = inverse[0][0] * c3.x() + inverse[0][1] * c3.y();
        const auto beta  = inverse[1][0] * c3.x() + inverse[1][1] * c3.y();
        constexpr auto isbetween01 = [](const T x){ return (zero <= x) && (x <= one); };
        if (isbetween01(alpha) && isbetween01(beta)) {
            return alpha * l1.first + (one - alpha) * l1.second;
        }
        return std::nullopt;
    }

}  // namespace msc
