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

#include "angular.hxx"

#include <algorithm>
#include <cmath>
#include <memory>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "math_constants.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(no_edges_no_angles)
    {
        for (auto n = 0; n < 10; ++n) {
            auto graph = std::make_unique<ogdf::Graph>();
            auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
            for (auto i = 0; i < n; ++i) {
                const auto v = graph->newNode();
                attrs->x(v) = 100.0 * i;
                attrs->y(v) = 0.0;
            }
            const auto angles = msc::get_all_angles_between_adjacent_incident_edges(*attrs);
            MSC_REQUIRE_EQ(0, angles.size());
        }
    }

    MSC_AUTO_TEST_CASE(on_edge_two_pi)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        attrs->x(v1) = 0.0;
        attrs->y(v1) = 0.0;
        attrs->x(v1) = 1.0;
        attrs->y(v1) = 0.0;
        const auto angles = msc::get_all_angles_between_adjacent_incident_edges(*attrs);
        MSC_REQUIRE_EQ(2, angles.size());
        MSC_REQUIRE_CLOSE(1.0E-10, 2.0 * M_PI, angles.front());
        MSC_REQUIRE_CLOSE(1.0E-10, 2.0 * M_PI, angles.back());
    }

    MSC_AUTO_TEST_CASE(zero_length_edges)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v1, v3);
        graph->newEdge(v1, v4);
        attrs->x(v1) =  0.0; attrs->y(v1) = 0.0;
        attrs->x(v2) = -1.0; attrs->y(v2) = 0.0;
        attrs->x(v3) = +1.0; attrs->y(v3) = 0.0;
        attrs->x(v4) =  0.0; attrs->y(v4) = 0.0;
        MSC_REQUIRE_EXCEPTION(
            std::logic_error,
            msc::get_all_angles_between_adjacent_incident_edges(*attrs, msc::treatments::exception)
        );
        const auto ignored = msc::get_all_angles_between_adjacent_incident_edges(*attrs, msc::treatments::ignore);
        const auto replaced = msc::get_all_angles_between_adjacent_incident_edges(*attrs, msc::treatments::replace);
        MSC_REQUIRE_EQ(4, ignored.size());
        MSC_REQUIRE_EQ(6, replaced.size());
        const auto is_pi1 = [](const double x){ return std::abs(x - 1.0 * M_PI) < 1.0E-10; };
        const auto is_pi2 = [](const double x){ return std::abs(x - 2.0 * M_PI) < 1.0E-10; };
        const auto is_nan = [](const double x){ return std::isnan(x); };
        MSC_REQUIRE_EQ(2, std::count_if(std::begin(ignored), std::end(ignored), is_pi1));
        MSC_REQUIRE_EQ(2, std::count_if(std::begin(ignored), std::end(ignored), is_pi2));
        MSC_REQUIRE_EQ(0, std::count_if(std::begin(ignored), std::end(ignored), is_nan));
        MSC_REQUIRE_EQ(1, std::count_if(std::begin(replaced), std::end(replaced), is_pi1));
        MSC_REQUIRE_EQ(2, std::count_if(std::begin(replaced), std::end(replaced), is_pi2));
        MSC_REQUIRE_EQ(3, std::count_if(std::begin(replaced), std::end(replaced), is_nan));
    }

    // Our graph looks like this:
    //
    //  1----2----3----4
    //  |\   |
    //  | \  |
    //  |  \ |
    //  |   \|
    //  5    6    7
    //
    MSC_AUTO_TEST_CASE(nontrivial)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        const auto v5 = graph->newNode();
        const auto v6 = graph->newNode();
        const auto v7 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v4);
        graph->newEdge(v1, v5);
        graph->newEdge(v1, v6);
        graph->newEdge(v2, v6);
        attrs->x(v1) = 0.0;  attrs->y(v1) = 0.0;
        attrs->x(v2) = 1.0;  attrs->y(v2) = 0.0;
        attrs->x(v3) = 2.0;  attrs->y(v3) = 0.0;
        attrs->x(v4) = 3.0;  attrs->y(v4) = 0.0;
        attrs->x(v5) = 0.0;  attrs->y(v5) = 1.0;
        attrs->x(v6) = 1.0;  attrs->y(v6) = 1.0;
        attrs->x(v7) = 2.0;  attrs->y(v7) = 1.0;
        const auto deg2rad = [](const double x){ return (M_PI / 180) * x; };
        auto expected = std::vector<double> {
             45,  45, 270,     // v1
            180,  90,  90,     // v2
            360,               // v3
            180, 180,          // v4
            360,               // v5
            315,  45,          // v6
                               // v7
        };
        std::transform(std::cbegin(expected), std::cend(expected), std::begin(expected), deg2rad);
        std::sort(std::begin(expected), std::end(expected));
        auto actual = msc::get_all_angles_between_adjacent_incident_edges(*attrs);
        std::sort(std::begin(actual), std::end(actual));
        MSC_REQUIRE_EQ(expected.size(), actual.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            MSC_REQUIRE_CLOSE(1.0E-10, expected.at(i), actual.at(i));
        }
    }

    MSC_AUTO_TEST_CASE(star)
    {
        for (auto n = 1; n < 100; ++n) {
            auto graph = std::make_unique<ogdf::Graph>();
            auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
            const auto v = graph->newNode();
            attrs->x(v) = 0.0;
            attrs->y(v) = 0.0;
            for (auto i = 0; i < n; ++i) {
                const auto u = graph->newNode();
                graph->newEdge(v, u);
                const auto theta = i * 2.0 * M_PI / n;
                attrs->x(u) = std::sin(theta);
                attrs->y(u) = std::cos(theta);
            }
            auto angles = msc::get_all_angles_between_adjacent_incident_edges(*attrs);
            std::sort(std::begin(angles), std::end(angles));
            MSC_REQUIRE_EQ(2 * n, angles.size());
            for (auto i = 0; i < n; ++i) {
                MSC_REQUIRE_CLOSE(1.0E-10, 2.0 * M_PI / n, angles.at(i));
                MSC_REQUIRE_CLOSE(1.0E-10, 2.0 * M_PI, angles.at(n + i));
            }
        }
    }

}  // namespace /*anonymous*/
