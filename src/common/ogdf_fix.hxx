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
 * @file ogdf_fix.hxx
 *
 * @brief
 *     Replacements for missing or broken features of the OGDF.
 *
 */

#ifndef MSC_OGDF_FIX_HXX
#define MSC_OGDF_FIX_HXX

#include <utility>

#include "ogdf_fwd.hxx"
#include "point.hxx"

namespace msc
{

    /**
     * @brief
     *     Returns the coordinates of a vertex in a layout.
     *
     * The behavior is undefined if the vertex does not belong th the layout's graph.
     *
     * @param attrs
     *     vertex layout
     *
     * @param v
     *     vertex
     *
     * @returns
     *     `{attrs.x(v), attrs.y(v)}`
     *
     */
    msc::point2d get_coords(const ogdf::GraphAttributes& attrs, ogdf::node v);

    /**
     * @brief
     *     Returns a pair containing the coordinates of the lower-left and upper-right corner of the layout's
     *     rectangular bounding box.
     *
     * This function is a replacement for `ogdf::GraphAttributes::boundingBox()` which appears to do the wrong thing.
     *
     * @param attrs
     *     layout to get the bounding box for
     *
     * @returns
     *     pair of coordinates
     *
     */
    std::pair<msc::point2d, msc::point2d> get_bounding_box(const ogdf::GraphAttributes& attrs) noexcept;

    /**
     * @brief
     *     Returns the size (width and height) of the layout's rectangular bounding box.
     *
     * This function is a replacement for `ogdf::GraphAttributes::boundingBox()` which appears to do the wrong thing.
     *
     * @param attrs
     *     layout to get the bounding box for
     *
     * @returns
     *     pair of dimensions
     *
     */
    msc::point2d get_bounding_box_size(const ogdf::GraphAttributes& attrs) noexcept;

}  // namespace msc

#endif  // !defined(MSC_OGDF_FIX_HXX)
