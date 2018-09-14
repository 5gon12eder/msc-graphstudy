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
 * @file fingerprint.hxx
 *
 * @brief
 *     Graph and layout fingerprints.
 *
 */

#ifndef MSC_FINGERPRINT_HXX
#define MSC_FINGERPRINT_HXX

#include <string>

#include "ogdf_fwd.hxx"

namespace msc
{

    /**
     * @brief
     *     Returns a fixed-length string that only depends on the given graph and is unlikely to collide with the ID
     *     returned for any other graph.
     *
     * Note that isomorphic graphs with different labels will (almost certainly) not get the same ID.
     *
     * @param graph
     *     graph to obtain an ID for
     *
     * @returns
     *     almost unique ID for the graph
     *
     */
    std::string graph_fingerprint(const ogdf::Graph& graph);

    /**
     * @brief
     *     Returns a fixed-length string that only depends on the given layout and is unlikely to collide with the ID
     *     returned for any other layout.
     *
     * Note that isomorphic graphs with different labels will (almost certainly) not get the same ID.
     *
     * @param attrs
     *     layout to obtain an ID for
     *
     * @returns
     *     almost unique ID for the graph
     *
     */
    std::string layout_fingerprint(const ogdf::GraphAttributes& attrs);

}  // namespace msc

#endif  // !defined(MSC_FINGERPRINT_HXX)
