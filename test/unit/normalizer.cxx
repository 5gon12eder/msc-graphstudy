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

#include "normalizer.hxx"

#include <cmath>
#include <random>
#include <tuple>
#include <utility>

#include "math_constants.hxx"
#include "testaux/cube.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    const auto sqr = [](auto x){ return x * x; };

    std::pair<double, double> center_of_gravity(const ogdf::GraphAttributes& attrs)
    {
        auto cx = 0.0;
        auto cy = 0.0;
        for (const auto v : attrs.constGraph().nodes) {
            cx += attrs.x(v);
            cy += attrs.y(v);
        }
        return std::make_pair(cx, cy);
    }

    double average_edge_length(const ogdf::GraphAttributes& attrs)
    {
        auto sum = 0.0;
        for (const auto e : attrs.constGraph().edges) {
            const auto v1 = e->source();
            const auto v2 = e->target();
            sum += std::sqrt(sqr(attrs.x(v1) - attrs.x(v2)) + sqr(attrs.y(v1) - attrs.y(v2)));
        }
        return sum / attrs.constGraph().numberOfEdges();
    }

    MSC_AUTO_TEST_CASE(empty)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(0, graph->numberOfNodes());
        MSC_REQUIRE_EQ(0, graph->numberOfEdges());
    }

    MSC_AUTO_TEST_CASE(singleton)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v = graph->newNode();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v) = 14.0;
        attrs->y(v) = 92.0;
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(1, graph->numberOfNodes());
        MSC_REQUIRE_EQ(0, graph->numberOfEdges());
        MSC_REQUIRE_CLOSE(1.0e-15, 0.0, attrs->x(v));
        MSC_REQUIRE_CLOSE(1.0e-15, 0.0, attrs->y(v));
    }

    MSC_AUTO_TEST_CASE(pair)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = 0.0;
        attrs->y(v1) = 0.0;
        attrs->x(v2) = 3.0;
        attrs->y(v2) = 4.0;
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(2, graph->numberOfNodes());
        MSC_REQUIRE_EQ(1, graph->numberOfEdges());
        const auto x1 = attrs->x(v1);
        const auto y1 = attrs->y(v1);
        const auto x2 = attrs->x(v2);
        const auto y2 = attrs->y(v2);
        const auto d = 100.0;
        MSC_REQUIRE_CLOSE(1.0e-15, -0.30 * d, x1);
        MSC_REQUIRE_CLOSE(1.0e-15, +0.30 * d, x2);
        MSC_REQUIRE_CLOSE(1.0e-15, -0.40 * d, y1);
        MSC_REQUIRE_CLOSE(1.0e-15, +0.40 * d, y2);
    }

    MSC_AUTO_TEST_CASE(two_points)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = 0.0;
        attrs->y(v1) = 0.0;
        attrs->x(v2) = 3.0;
        attrs->y(v2) = 4.0;
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(2, graph->numberOfNodes());
        MSC_REQUIRE_EQ(0, graph->numberOfEdges());
        const auto x1 = attrs->x(v1);
        const auto y1 = attrs->y(v1);
        const auto x2 = attrs->x(v2);
        const auto y2 = attrs->y(v2);
        const auto d = 100.0;
        MSC_REQUIRE_CLOSE(1.0e-15, -0.30 * d, x1);
        MSC_REQUIRE_CLOSE(1.0e-15, +0.30 * d, x2);
        MSC_REQUIRE_CLOSE(1.0e-15, -0.40 * d, y1);
        MSC_REQUIRE_CLOSE(1.0e-15, +0.40 * d, y2);
    }

    MSC_AUTO_TEST_CASE(three_points)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) =  0.0; attrs->y(v1) =  0.0;
        attrs->x(v2) = 10.0; attrs->y(v2) =  0.0;
        attrs->x(v3) =  5.0; attrs->y(v3) = 10.0 * std::sin(M_PI / 3.0);
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(3, graph->numberOfNodes());
        MSC_REQUIRE_EQ(0, graph->numberOfEdges());
        const auto x1 = attrs->x(v1);  const auto y1 = attrs->y(v1);
        const auto x2 = attrs->x(v2);  const auto y2 = attrs->y(v2);
        const auto x3 = attrs->x(v3);  const auto y3 = attrs->y(v3);
        MSC_REQUIRE_CLOSE(1.0e-10, -100.0 / 2.0, x1);
        MSC_REQUIRE_CLOSE(1.0e-10, +100.0 / 2.0, x2);
        MSC_REQUIRE_CLOSE(1.0e-10,   0.0, x3);
        MSC_REQUIRE_CLOSE(1.0e-10, -100.0 * std::sin(M_PI / 3.0) / 3.0, y1);
        MSC_REQUIRE_CLOSE(1.0e-10, -100.0 * std::sin(M_PI / 3.0) / 3.0, y2);
        MSC_REQUIRE_CLOSE(1.0e-10,  200.0 * std::sin(M_PI / 3.0) / 3.0, y3);
    }

    MSC_AUTO_TEST_CASE(cube)
    {
        auto [graph, attrs] = msc::test::make_cube_layout();
        const auto nodes = graph->numberOfNodes();
        const auto edges = graph->numberOfEdges();
        msc::normalize_layout(*attrs);
        MSC_REQUIRE_EQ(nodes, graph->numberOfNodes());
        MSC_REQUIRE_EQ(edges, graph->numberOfEdges());
        const auto [cx, cy] = center_of_gravity(*attrs);
        MSC_REQUIRE_CLOSE(1.0e-15, 0.0, cx);
        MSC_REQUIRE_CLOSE(1.0e-15, 0.0, cy);
        MSC_REQUIRE_CLOSE(1.0e-15, 100.0, average_edge_length(*attrs));
    }

}  // namespace /*anonymous*/
