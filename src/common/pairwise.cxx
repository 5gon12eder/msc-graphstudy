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

#include "pairwise.hxx"

#include <limits>

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/graphalg/ShortestPathAlgorithms.h>

// Make sure we've actually included the OGDF headers as advertised in the DocString instead of just forward-declaring
// the types as we usually do.

static_assert(sizeof(ogdf::Graph) > 0);
static_assert(sizeof(ogdf::GraphAttributes) > 0);

namespace msc
{

    namespace /*anonymous*/
    {

        std::unique_ptr<ogdf_node_array_2d<double>>
        make_ogdf_node_array_2d_double(const ogdf::Graph& graph)
        {
            const auto initval = std::numeric_limits<double>::quiet_NaN();
            return std::make_unique<ogdf_node_array_2d<double>>(graph, ogdf_node_array_1d<double>{graph, initval});
        }

    }  // namespace /*anonymous*/

    std::unique_ptr<ogdf_node_array_2d<double>> get_pairwise_shortest_paths(const ogdf::Graph& graph)
    {
        auto matrix = make_ogdf_node_array_2d_double(graph);
        const auto unitweights = ogdf::EdgeArray<double>{graph, 1.0};
        ogdf::dijkstra_SPAP(graph, *matrix, unitweights);
        return matrix;
    }

}  // namespace msc
