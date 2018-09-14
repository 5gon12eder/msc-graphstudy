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

#include "rdf.hxx"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "testaux/cube.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    struct closeto
    {
        double expected{};
        bool operator()(const double actual) const noexcept { return std::abs(actual - expected) <= 1.0E-10; };
    };

    MSC_AUTO_TEST_CASE(node_distance)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph, ogdf::GraphAttributes::nodeGraphics);
        attrs->x(v1) = -1.0;
        attrs->y(v1) = -1.0;
        attrs->x(v2) = +2.0;
        attrs->y(v2) = +3.0;
        const auto proj = msc::node_distance{*attrs};
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, proj(v1, v1));
        MSC_REQUIRE_CLOSE(1.0E-10, 5.0, proj(v1, v2));
        MSC_REQUIRE_CLOSE(1.0E-10, 5.0, proj(v2, v1));
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, proj(v2, v2));
    }

    MSC_AUTO_TEST_CASE(global_pairwise_distances_square)
    {
        const auto [graph, attrs] = msc::test::make_square_layout();
        const auto gpd = msc::global_pairwise_distances{*attrs};
        MSC_REQUIRE_EQ(6, std::distance(std::begin(gpd), std::end(gpd)));
        MSC_REQUIRE_EQ(4, std::count_if(std::begin(gpd), std::end(gpd), closeto{100.0}));
        MSC_REQUIRE_EQ(2, std::count_if(std::begin(gpd), std::end(gpd), closeto{100.0 * std::sqrt(2.0)}));
        (void) graph.get();
    }

    MSC_AUTO_TEST_CASE(local_pairwise_distances_square_0)
    {
        const auto [graph, attrs] = msc::test::make_square_layout();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto lpd = msc::local_pairwise_distances{*attrs, *matrix, 0.5};
        MSC_REQUIRE_EQ(0, std::distance(std::begin(lpd), std::end(lpd)));
    }

    MSC_AUTO_TEST_CASE(local_pairwise_distances_square_1)
    {
        const auto [graph, attrs] = msc::test::make_square_layout();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto lpd = msc::local_pairwise_distances{*attrs, *matrix, 1.5};
        MSC_REQUIRE_EQ(4, std::distance(std::begin(lpd), std::end(lpd)));
        for (const auto distance : lpd) {
            MSC_REQUIRE_CLOSE(1.0E-10, 100.0, distance);
        }
    }

    MSC_AUTO_TEST_CASE(local_pairwise_distances_square_2)
    {
        const auto [graph, attrs] = msc::test::make_square_layout();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto lpd = msc::local_pairwise_distances{*attrs, *matrix, 2.5};
        MSC_REQUIRE_EQ(6, std::distance(std::begin(lpd), std::end(lpd)));
        MSC_REQUIRE_EQ(4, std::count_if(std::begin(lpd), std::end(lpd), closeto{100.0}));
        MSC_REQUIRE_EQ(2, std::count_if(std::begin(lpd), std::end(lpd), closeto{100.0 * std::sqrt(2.0)}));
    }

}  // namespace /*anonymous*/
