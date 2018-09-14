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
 * @file normalizer.hxx
 *
 * @brief
 *     Normalized layouts.
 *
 */

#ifndef MSC_NORMALIZER_HXX
#define MSC_NORMALIZER_HXX

#include "ogdf_fwd.hxx"

namespace msc
{

    /** @brief Average edge length in normalized layouts. */
    inline constexpr auto default_node_distance = 100.0;

    /**
     * @brief
     *     &ldquo;Normalizes&rdquo; a layout.
     *
     * A normalized layout will be translated to have its center of gravity at the origin and scaled to have an average
     * edge length of 100.
     *
     * @param attrs
     *     layout to be normalized
     *
     */
    void normalize_layout(ogdf::GraphAttributes& attrs);

}  // namespace msc

#endif  // !defined(MSC_NORMALIZER_HXX)
