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
#  include <config.h>
#endif

#define MSC_RUN_ALL_UNIT_TESTS_IN_MAIN

#include "edge_length.hxx"

#include <map>
#include <memory>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(no_edges_no_lengths)
    {
        for (auto n = 0; n < 10; ++n) {
            auto graph = std::make_unique<ogdf::Graph>();
            auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
            for (auto i = 0; i < n; ++i) {
                const auto v = graph->newNode();
                attrs->x(v) = 100.0 * i;
                attrs->y(v) = 0.0;
            }
            const auto lengths = msc::get_all_edge_lengths(*attrs);
            MSC_REQUIRE_EQ(0, lengths.size());
        }
    }

    MSC_AUTO_TEST_CASE(single_pair)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        attrs->x(v1) =   0.0;
        attrs->y(v1) =   0.0;
        attrs->x(v2) = 300.0;
        attrs->y(v2) = 400.0;
        const auto lengths = msc::get_all_edge_lengths(*attrs);
        MSC_REQUIRE_EQ(1, lengths.size());
        MSC_REQUIRE_CLOSE(1.0E-10, 500.0, lengths.front());
    }

    MSC_AUTO_TEST_CASE(square)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v4);
        graph->newEdge(v4, v1);
        attrs->x(v1) =   0.0; attrs->y(v1) =   0.0;
        attrs->x(v2) =   0.0; attrs->y(v2) = 100.0;
        attrs->x(v3) = 100.0; attrs->y(v3) = 100.0;
        attrs->x(v4) = 100.0; attrs->y(v4) =   0.0;
        const auto lengths = msc::get_all_edge_lengths(*attrs);
        MSC_REQUIRE_EQ(4, lengths.size());
        for (const auto length : lengths) {
            MSC_REQUIRE_CLOSE(1.0E-10, 100.0, length);
        }
    }

    MSC_AUTO_TEST_CASE(square_with_diagonals)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v4);
        graph->newEdge(v4, v1);
        graph->newEdge(v1, v3);
        graph->newEdge(v2, v4);
        attrs->x(v1) =   0.0; attrs->y(v1) =   0.0;
        attrs->x(v2) =   0.0; attrs->y(v2) = 100.0;
        attrs->x(v3) = 100.0; attrs->y(v3) = 100.0;
        attrs->x(v4) = 100.0; attrs->y(v4) =   0.0;
        const auto lengths = msc::get_all_edge_lengths(*attrs);
        MSC_REQUIRE_EQ(6, lengths.size());
        auto counting = std::map<double, int>{};
        for (const auto length : lengths) {
            counting[std::round(length)] += 1;
        }
        MSC_REQUIRE_EQ(2, counting.size());
        MSC_REQUIRE_EQ(4, counting.at(100.0));
        MSC_REQUIRE_EQ(2, counting.at(141.0));
    }

}  // namespace /*anonymous*/
