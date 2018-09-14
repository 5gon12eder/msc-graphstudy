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
 * @file edge_length.hxx
 *
 * @brief
 *     Computing edge lengths.
 *
 */

#ifndef MSC_EDGE_LENGTH_HXX
#define MSC_EDGE_LENGTH_HXX

#include <vector>

#include "ogdf_fwd.hxx"

namespace msc
{

    /**
     * @brief
     *     Computes all edge lengths.
     *
     * @param attrs
     *     layout to operate on
     *
     * @returns
     *     edge lengths in unspecified order
     *
     */
    std::vector<double> get_all_edge_lengths(const ogdf::GraphAttributes& attrs);

}  // namespace msc

#endif  // !defined(MSC_EDGE_LENGTH_HXX)
