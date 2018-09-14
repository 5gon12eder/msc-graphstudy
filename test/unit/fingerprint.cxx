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

#include "fingerprint.hxx"

#include <memory>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "point.hxx"
#include "testaux/cube.hxx"
#include "unittest.hxx"

namespace /*anonynmous*/
{

    MSC_AUTO_TEST_CASE(fp1st)
    {
        const auto [graph, attrs] = msc::test::make_cube_layout();
        MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph), msc::graph_fingerprint(*graph));
        MSC_REQUIRE_EQ(msc::layout_fingerprint(*attrs), msc::layout_fingerprint(*attrs));
        MSC_REQUIRE_NE(msc::graph_fingerprint(*graph), msc::layout_fingerprint(*attrs));
    }

    MSC_AUTO_TEST_CASE(fp2nd)
    {
        const auto [graph, attrs] = msc::test::make_cube_layout();
        const auto graph_before = msc::graph_fingerprint(*graph);
        const auto layout_before = msc::layout_fingerprint(*attrs);
        attrs->scale(2.0, 2.0);
        const auto graph_after = msc::graph_fingerprint(*graph);
        const auto layout_after = msc::layout_fingerprint(*attrs);
        MSC_REQUIRE_EQ(graph_before, graph_after);
        MSC_REQUIRE_NE(layout_before, layout_after);
    }

    MSC_AUTO_TEST_CASE(fp3rd)
    {
        const auto [graph, attrs] = msc::test::make_cube_layout();
        const auto graph_before = msc::graph_fingerprint(*graph);
        const auto layout_before = msc::layout_fingerprint(*attrs);
        graph->newNode();
        const auto graph_after = msc::graph_fingerprint(*graph);
        const auto layout_after = msc::layout_fingerprint(*attrs);
        MSC_REQUIRE_NE(graph_before, graph_after);
        MSC_REQUIRE_NE(layout_before, layout_after);
    }

}  // namespace /*anonymous*/
