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

#include "ogdf_fix.hxx"

#include <cmath>
#include <memory>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(get_coords)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        attrs->x(v1) = 1.0; attrs->y(v1) = 2.0;
        attrs->x(v2) = 3.0; attrs->y(v2) = 4.0;
        MSC_REQUIRE_EQ(msc::point2d(1.0, 2.0), msc::get_coords(*attrs, v1));
        MSC_REQUIRE_EQ(msc::point2d(3.0, 4.0), msc::get_coords(*attrs, v2));
    }

    MSC_AUTO_TEST_CASE(bbox_empty)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto [sw, ne] = msc::get_bounding_box(*attrs);
        const auto size = msc::get_bounding_box_size(*attrs);
        MSC_REQUIRE(std::isnan(sw.x()) && std::isnan(sw.y()));
        MSC_REQUIRE(std::isnan(ne.x()) && std::isnan(ne.y()));
        MSC_REQUIRE(std::isnan(size.x()) && std::isnan(size.y()));
    }

    MSC_AUTO_TEST_CASE(bbox_singleton)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v = graph->newNode();
        attrs->x(v) = 1.4; attrs->y(v) = 9.2;
        const auto [sw, ne] = msc::get_bounding_box(*attrs);
        const auto size = msc::get_bounding_box_size(*attrs);
        MSC_REQUIRE_EQ(msc::point2d(1.4, 9.2), sw);
        MSC_REQUIRE_EQ(msc::point2d(1.4, 9.2), ne);
        MSC_REQUIRE_EQ(msc::point2d(0.0, 0.0), size);
    }

    MSC_AUTO_TEST_CASE(bbox_triangle)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v1);
        attrs->x(v1) = -1.0; attrs->y(v1) = -1.0;
        attrs->x(v2) = +2.0; attrs->y(v2) = +0.5;
        attrs->x(v3) = +0.5; attrs->y(v3) = +2.5;
        const auto [sw, ne] = msc::get_bounding_box(*attrs);
        const auto size = msc::get_bounding_box_size(*attrs);
        MSC_REQUIRE_EQ(msc::point2d(-1.0, -1.0), sw);
        MSC_REQUIRE_EQ(msc::point2d(+2.0, +2.5), ne);
        MSC_REQUIRE_EQ(msc::point2d(3.0, 3.5), size);
    }

    MSC_AUTO_TEST_CASE(bbox_moved)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        attrs->x(v1) = 14.0; attrs->y(v1) = -5.0;
        attrs->x(v2) = 92.0; attrs->y(v2) = -7.0;
        const auto [sw, ne] = msc::get_bounding_box(*attrs);
        const auto size = msc::get_bounding_box_size(*attrs);
        MSC_REQUIRE_EQ(msc::point2d(14.0, -7.0), sw);
        MSC_REQUIRE_EQ(msc::point2d(92.0, -5.0), ne);
        MSC_REQUIRE_EQ(msc::point2d(78.0, 2.0), size);
    }

}  // namespace /*anonymous*/
