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

#include "fingerprint.hxx"

#include <cassert>
#include <random>
#include <utility>
#include <vector>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "random.hxx"

namespace msc
{

    std::string graph_fingerprint(const ogdf::Graph& graph)
    {
        auto thevalues = std::vector<int>{};
        thevalues.push_back(graph.numberOfNodes());
        thevalues.push_back(graph.numberOfEdges());
        for (const auto node : graph.nodes) {
            thevalues.push_back(node->index());
        }
        for (const auto edge : graph.edges) {
            thevalues.push_back(edge->source()->index());
            thevalues.push_back(edge->target()->index());
        }
        auto seedseq = std::seed_seq(std::cbegin(thevalues), std::cend(thevalues));
        auto rndeng = std::mt19937{seedseq};
        return random_hex_string(rndeng);
    }

    std::string layout_fingerprint(const ogdf::GraphAttributes& attrs)
    {
        assert(attrs.has(ogdf::GraphAttributes::nodeGraphics));
        assert(attrs.has(ogdf::GraphAttributes::edgeGraphics));
        auto thevalues = std::vector<double>{};
        for (const auto node : attrs.constGraph().nodes) {
            thevalues.push_back(attrs.x(node));
            thevalues.push_back(attrs.y(node));
        }
        for (const auto edge : attrs.constGraph().edges) {
            thevalues.push_back(attrs.x(edge->source()));
            thevalues.push_back(attrs.y(edge->source()));
            thevalues.push_back(attrs.x(edge->target()));
            thevalues.push_back(attrs.y(edge->target()));
        }
        auto seedseq = std::seed_seq(std::cbegin(thevalues), std::cend(thevalues));
        auto rndeng = std::mt19937{seedseq};
        return random_hex_string(rndeng);
    }

}  // namespace msc
