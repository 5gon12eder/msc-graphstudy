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
 * @file edge_crossing.hxx
 *
 * @brief
 *     Edge crossing algorithms.
 *
 */

#ifndef MSC_EDGE_CROSSING_HXX
#define MSC_EDGE_CROSSING_HXX

#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "ogdf_fwd.hxx"
#include "point.hxx"

namespace msc
{

    /**
     * @brief
     *     Planar line represented by its start and end points.
     *
     * @tparam T
     *     floating-point type used to represent coordinates
     *
     */
    template <typename T>
    using planar_line = std::pair<point<T, 2>, point<T, 2>>;

    /**
     * @brief
     *     Finds the coordinates of all (real) edge crossings (between non-adjacent edges) in a layout.
     *
     * @param attrs
     *     layout to find edge crossings in
     *
     * @returns
     *     list of triples `(p, e1, e2)` of the coordinates `p` of the intersection between edges `e1` and `e2`
     *
     */
    auto find_edge_crossings(const ogdf::GraphAttributes& attrs)
        -> std::vector<std::tuple<point2d, ogdf::edge, ogdf::edge>>;

    /**
     * @brief
     *     Returns the crossing angle between edges `e` and `e2` in layout `attrs`.
     *
     * The behavior is undefined if `e1` and `e2` are not edges in `attrs.getConstGraph()` or don't intersect.
     *
     * @param attrs
     *     layout to consider
     *
     * @param e1
     *     first edge
     *
     * @param e2
     *     second edge
     *
     * @returns
     *     angle between `e1` and `e2`
     *
     */
    auto get_crossing_angle(const ogdf::GraphAttributes& attrs, ogdf::edge e1, ogdf::edge e2) -> double;

    /**
     * @brief
     *     Determines wheter two planar lines intersect each other.
     *
     * @tparam T
     *     floating-point type for coordinates
     *
     * @param l1
     *     first line
     *
     * @param l2
     *     second line
     *
     * @returns
     *     the coordinates of the intersection point if it exists
     *
     */
    template <typename T>
    constexpr auto check_intersect(planar_line<T> l1, planar_line<T> l2) noexcept
        -> std::optional<point<T, 2>>;

}  // namespace msc

#define MSC_INCLUDED_FROM_EDGE_CROSSING_HXX
#include "edge_crossing.txx"
#undef MSC_INCLUDED_FROM_EDGE_CROSSING_HXX

#endif  // !defined(MSC_EDGE_CROSSING_HXX)
