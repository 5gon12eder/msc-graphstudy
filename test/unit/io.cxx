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

#include "io.hxx"

#include <cmath>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "file.hxx"
#include "fingerprint.hxx"
#include "ogdf_fix.hxx"
#include "testaux/cube.hxx"
#include "testaux/tempfile.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    void require_graphs_close(const ogdf::Graph& lhs, const ogdf::Graph& rhs)
    {
        MSC_REQUIRE_EQ(lhs.numberOfNodes(), rhs.numberOfNodes());
        MSC_REQUIRE_EQ(lhs.numberOfEdges(), rhs.numberOfEdges());
        const auto extract_degrees = [](const ogdf::Graph& g) {
            auto degrees = std::map<int, int>{};
            for (const auto v : g.nodes) {
                degrees[v->index()] += 1;
            }
            return degrees;
        };
        MSC_REQUIRE_EQ(extract_degrees(lhs), extract_degrees(rhs));
    }

    void require_layouts_close(const ogdf::GraphAttributes& lhs, const ogdf::GraphAttributes& rhs)
    {
        require_graphs_close(lhs.constGraph(), rhs.constGraph());
        const auto lhsbb = msc::get_bounding_box(lhs);
        const auto rhsbb = msc::get_bounding_box(lhs);
        MSC_REQUIRE_CLOSE(1.0E-3, lhsbb.first.x(), rhsbb.first.x());
        MSC_REQUIRE_CLOSE(1.0E-3, lhsbb.first.y(), rhsbb.first.y());
        MSC_REQUIRE_CLOSE(1.0E-3, lhsbb.second.x(), rhsbb.second.x());
        MSC_REQUIRE_CLOSE(1.0E-3, lhsbb.second.y(), rhsbb.second.y());
        const auto extract_coords = [](const ogdf::GraphAttributes& attrs) {
            auto coords = std::map<std::pair<double, double>, int>{};
            for (const auto v : attrs.constGraph().nodes) {
                const auto x = std::round(attrs.x(v));
                const auto y = std::round(attrs.y(v));
                coords[std::make_pair(x, y)] += 1;
            }
            return coords;
        };
        MSC_REQUIRE_EQ(extract_coords(lhs), extract_coords(rhs));
    }

    MSC_AUTO_TEST_CASE(roundtrip_graph_formats)
    {
        using namespace std::string_literals;
        auto tally = 0;
        const auto cubegraph = msc::test::make_cube_graph();
        for (const auto fmt : msc::all_fileformats()) {
            const auto tmp = msc::test::tempfile{};
            const auto file = msc::file::from_filename(tmp.filename(), msc::compressions::none);
            try {
                msc::export_graph(*cubegraph, file, fmt);
            } catch (const msc::unsupported_format& e) {
                MSC_REQUIRE_MATCH(".*write.*graph.*"s + name(fmt).data() + ".*"s, e.what());
                continue;
            }
            const auto graph1st = msc::import_graph(file, fmt);
            msc::export_graph(*graph1st, file, fmt);
            const auto graph2nd = msc::import_graph(file, fmt);
            require_graphs_close(*cubegraph, *graph1st);
            require_graphs_close(*cubegraph, *graph2nd);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph2nd));
            ++tally;
        }
        MSC_REQUIRE_GE(tally, 13);
    }

    MSC_AUTO_TEST_CASE(roundtrip_layout_formats)
    {
        using namespace std::string_literals;
        auto tally = 0;
        const auto [cubegraph, cubeattrs] = msc::test::make_cube_layout();
        for (const auto fmt : msc::all_fileformats()) {
            const auto tmp = msc::test::tempfile{};
            const auto file = msc::file::from_filename(tmp.filename(), msc::compressions::none);
            try {
                msc::export_layout(*cubeattrs, file, fmt);
            } catch (const msc::unsupported_format& e) {
                MSC_REQUIRE_MATCH(".*write.*layout.*"s + name(fmt).data() + ".*"s, e.what());
                continue;
            }
            const auto [graph1st, attrs1st] = msc::import_layout(file, fmt);
            msc::export_layout(*attrs1st, file, fmt);
            const auto [graph2nd, attrs2nd] = msc::import_layout(file, fmt);
            require_layouts_close(*cubeattrs, *attrs1st);
            require_layouts_close(*cubeattrs, *attrs2nd);
            MSC_REQUIRE_EQ(msc::layout_fingerprint(*attrs1st), msc::layout_fingerprint(*attrs2nd));
            const auto graph3rd = msc::import_graph(file, fmt);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph3rd));
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph2nd), msc::graph_fingerprint(*graph3rd));
            require_graphs_close(*cubegraph, *graph3rd);
            ++tally;
        }
        MSC_REQUIRE_GE(tally, 6);
    }

    MSC_AUTO_TEST_CASE(roundtrip_default_graph_uncompressed)
    {
        for (const auto comp : {msc::compressions::automatic, msc::compressions::none}) {
            const auto tmp = msc::test::tempfile{".xml"};
            const auto file = msc::file::from_filename(tmp.filename(), comp);
            const auto graph1st = msc::test::make_cube_graph();
            msc::store_graph(*graph1st, file);
            const auto graph2nd = msc::load_graph(file);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph2nd));
        }
    }

    MSC_AUTO_TEST_CASE(roundtrip_default_graph_compressed)
    {
        for (const auto comp : {msc::compressions::automatic, msc::compressions::gzip}) {
            const auto tmp = msc::test::tempfile{".xml.gz"};
            const auto file = msc::file::from_filename(tmp.filename(), comp);
            const auto graph1st = msc::test::make_cube_graph();
            msc::store_graph(*graph1st, file);
            const auto graph2nd = msc::load_graph(file);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph2nd));
        }
    }

    MSC_AUTO_TEST_CASE(roundtrip_default_layout_uncompressed)
    {
        for (const auto comp : {msc::compressions::automatic, msc::compressions::none}) {
            const auto tmp = msc::test::tempfile{".xml"};
            const auto file = msc::file::from_filename(tmp.filename(), comp);
            const auto [graph1st, attrs1st] = msc::test::make_cube_layout();
            msc::store_layout(*attrs1st, file);
            const auto [graph2nd, attrs2nd] = msc::load_layout(file);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph2nd));
            MSC_REQUIRE_EQ(msc::layout_fingerprint(*attrs1st), msc::layout_fingerprint(*attrs2nd));
        }
    }

    MSC_AUTO_TEST_CASE(roundtrip_default_layout_compressed)
    {
        for (const auto comp : {msc::compressions::automatic, msc::compressions::gzip}) {
            const auto tmp = msc::test::tempfile{".xml.gz"};
            const auto file = msc::file::from_filename(tmp.filename(), comp);
            const auto [graph1st, attrs1st] = msc::test::make_cube_layout();
            msc::store_layout(*attrs1st, file);
            const auto [graph2nd, attrs2nd] = msc::load_layout(file);
            MSC_REQUIRE_EQ(msc::graph_fingerprint(*graph1st), msc::graph_fingerprint(*graph2nd));
            MSC_REQUIRE_EQ(msc::layout_fingerprint(*attrs1st), msc::layout_fingerprint(*attrs2nd));
        }
    }

    MSC_AUTO_TEST_CASE(degenerate_layout_rejected)
    {
        const auto tmp = msc::test::tempfile{};
        const auto file = msc::file::from_filename(tmp.filename(), msc::compressions::none);
        const auto graph = msc::test::make_cube_graph();
        msc::store_graph(*graph, file);
        MSC_REQUIRE_EXCEPTION(msc::degenerated_layout, msc::load_layout(file));
        MSC_REQUIRE_EXCEPTION(msc::degenerated_layout, msc::import_layout(file, msc::internal_file_format));
        msc::load_graph(file);  // this should be fine
        msc::import_graph(file, msc::internal_file_format);  // this should be fine
    }

}  // namespace /*anonymous*/
