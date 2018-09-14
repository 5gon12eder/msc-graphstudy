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

#include "tension.hxx"

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "math_constants.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    template
    <
        typename FwdIterT,
        typename ValueT = std::decay_t<typename std::iterator_traits<FwdIterT>::value_type>
    >
    std::vector<double> sorted(const FwdIterT first, const FwdIterT last)
    {
        static_assert(std::is_same_v<ValueT, double>);
        auto values = std::vector<double>(first, last);
        std::sort(values.begin(), values.end());
        return values;
    }

    MSC_AUTO_TEST_CASE(empty_graph)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto tension = msc::pairwise_tension{*attrs, *matrix, 1.0};
        MSC_REQUIRE(std::begin(tension) == std::end(tension));
    }

    MSC_AUTO_TEST_CASE(singleton_graph)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        graph->newNode();
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto tension = msc::pairwise_tension{*attrs, *matrix, 1.0};
        MSC_REQUIRE(std::begin(tension) == std::end(tension));
    }

    MSC_AUTO_TEST_CASE(pair)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = 1.0;  attrs->y(v1) = 1.0;
        attrs->x(v2) = 2.0;  attrs->y(v2) = 2.0;
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto tension = msc::pairwise_tension{*attrs, *matrix, 2.0};
        const auto values = sorted(std::begin(tension), std::end(tension));
        MSC_REQUIRE_EQ(1, values.size());
        MSC_REQUIRE_CLOSE(1.0E-10, M_SQRT2, values.front());
    }

    MSC_AUTO_TEST_CASE(pair_disconnected)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = 1.0;  attrs->y(v1) = 1.0;
        attrs->x(v2) = 2.0;  attrs->y(v2) = 2.0;
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto tension = msc::pairwise_tension{*attrs, *matrix, 0.9};
        MSC_REQUIRE(std::begin(tension) == std::end(tension));
    }

    MSC_AUTO_TEST_CASE(four)
    {
        //  A --- B --- C
        //  |           |
        //  +---- D ----+
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v4);
        graph->newEdge(v4, v1);
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = -10.0;  attrs->y(v1) =  0.0;
        attrs->x(v2) =   0.0;  attrs->y(v2) =  0.0;
        attrs->x(v3) = +10.0;  attrs->y(v3) =  0.0;
        attrs->x(v4) =   0.0;  attrs->y(v4) = 10.0;
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto tension = msc::pairwise_tension{*attrs, *matrix, 2.0};
        const auto values = sorted(std::begin(tension), std::end(tension));
        MSC_REQUIRE_EQ(6, values.size());
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(0), 10.0 / 2.0);      // v2 v4
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(1), 10.0);            // v1 v2
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(2), 10.0);            // v2 v3
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(3), 10.0);            // v1 v3
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(4), 10.0 * M_SQRT2);  // v1 v4
        MSC_REQUIRE_CLOSE(1.0E-10, values.at(5), 10.0 * M_SQRT2);  // v3 v4
    }

}  // namespace /*anonymous*/
