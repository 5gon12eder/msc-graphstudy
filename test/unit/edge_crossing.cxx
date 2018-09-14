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

#include "edge_crossing.hxx"

#include <cstdlib>
#include <stdexcept>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/graphics.h>
#include <ogdf/fileformats/GraphIO.h>

#include <boost/iostreams/filtering_stream.hpp>

#include "file.hxx"
#include "iosupp.hxx"
#include "testaux/cube.hxx"
#include "unittest.hxx"

#ifndef NDEBUG
#  define REQUIRE_INTERSECTION( ... ) MSC_REQUIRE(check( __VA_ARGS__ ))
#else
#  define REQUIRE_INTERSECTION( ... ) static_assert(check_cx( __VA_ARGS__ ))
#endif

namespace /*anonymous*/
{

    constexpr auto get_segment_permutation(const msc::planar_line<double>& l1,
                                           const msc::planar_line<double>& l2,
                                           const int counter) noexcept
        -> std::pair<msc::planar_line<double>, msc::planar_line<double>>
    {
        const auto [a, b] = l1;
        const auto [c, d] = l2;
        switch (counter) {
        case 0: return {{a, b}, {c, d}};
        case 1: return {{a, b}, {d, c}};
        case 2: return {{b, a}, {c, d}};
        case 3: return {{b, a}, {d, c}};
        case 4: return {{c, d}, {a, b}};
        case 5: return {{d, c}, {a, b}};
        case 6: return {{c, d}, {b, a}};
        case 7: return {{d, c}, {b, a}};
        }
        const auto wtf = msc::make_invalid_point<double, 2>();
        return {{wtf, wtf}, {wtf, wtf}};
    }

    constexpr bool check_cx(const msc::planar_line<double>& l1,
                            const msc::planar_line<double>& l2,
                            const std::optional<msc::point2d>& expected,
                            const int counter = 0) noexcept
    {
        const auto [lhs, rhs] = get_segment_permutation(l1, l2, counter);
        const auto actual = msc::check_intersect(lhs, rhs);
        if (actual.has_value() != expected.has_value()) {
            return false;
        }
        if (expected.has_value() && actual.has_value()) {
            if (msc::normsq(actual.value() - expected.value()) > 1.0E-5) {
                return false;
            }
        }
        return (counter + 1 < 8) ? check_cx(l1, l2, expected, counter + 1) : true;
    }

    bool check(const msc::planar_line<double>& l1,
               const msc::planar_line<double>& l2,
               const std::optional<msc::point2d>& expected,
               const int counter = 0)
    {
        const auto [lhs, rhs] = get_segment_permutation(l1, l2, counter);
        const auto actual = msc::check_intersect(lhs, rhs);
        if (!expected) {
            if (actual) {
                std::clog << "Expected no intersection of "
                          << l1.first << " -- " << l1.second << " and "
                          << l2.first << " -- " << l2.second << " but found intersection at "
                          << actual.value() << std::endl;
                return false;
            }
        } else {
            if (!actual) {
                std::clog << "Expected intersection of "
                          << l1.first << " -- " << l1.second << " and "
                          << l2.first << " -- " << l2.second << " around "
                          << expected.value() << " but none was found" << std::endl;
                return false;
            } else if (distance(actual.value(), expected.value()) > 1.0E-10) {
                std::clog << "Expected intersection of "
                          << l1.first << " -- " << l1.second << " and "
                          << l2.first << " -- " << l2.second << " around "
                          << expected.value() << " but was found at " << actual.value()
                          << " which is " << distance(actual.value(), expected.value()) << " away" << std::endl;
                return false;
            }
        }
        return (counter + 1 < 8) ? check(l1, l2, expected, counter + 1) : true;
    }

    MSC_AUTO_TEST_CASE(intersection)
    {
        REQUIRE_INTERSECTION({{4.0, 9.0}, {4.0, 9.0}}, {{4.0, 9.0}, {4.0, 9.0}}, {{4.0, 9.0}});  // both zero coinciding
        REQUIRE_INTERSECTION({{4.0, 9.0}, {4.0, 9.0}}, {{9.0, 4.0}, {9.0, 4.0}}, std::nullopt);  // both zero different
        REQUIRE_INTERSECTION({{4.0, 9.0}, {4.0, 9.0}}, {{9.0, 4.0}, {9.0, 4.0}}, std::nullopt);  // both zero different
        REQUIRE_INTERSECTION({{0.0, 1.0}, {3.0, 1.0}}, {{2.0, 1.0}, {2.0, 1.0}}, {{2.0, 1.0}});  // one zero inside
        REQUIRE_INTERSECTION({{0.0, 1.0}, {2.0, 1.0}}, {{3.0, 1.0}, {3.0, 1.0}}, std::nullopt);  // one zero outside
        REQUIRE_INTERSECTION({{0.0, 1.0}, {3.0, 1.0}}, {{2.0, 1.5}, {2.0, 1.5}}, std::nullopt);  // one zero off
        REQUIRE_INTERSECTION({{0.0, 1.0}, {3.0, 1.0}}, {{3.0, 1.0}, {3.0, 1.0}}, {{3.0, 1.0}});  // one zero begin

        REQUIRE_INTERSECTION({{0.0, 0.0}, {2.0, 0.0}}, {{3.0, 0.0}, {5.0, 0.0}}, std::nullopt);  // colinear with gap
        REQUIRE_INTERSECTION({{0.0, 1.0}, {2.0, 1.0}}, {{2.0, 1.0}, {3.0, 1.0}}, {{2.0, 1.0}});  // colinear touch
        REQUIRE_INTERSECTION({{0.0, 1.0}, {2.0, 1.0}}, {{1.0, 1.0}, {3.0, 1.0}}, {{1.5, 1.0}});  // partial overlap
        REQUIRE_INTERSECTION({{0.0, 0.0}, {3.0, 6.0}}, {{1.0, 2.0}, {2.0, 4.0}}, {{1.5, 3.0}});  // subset overlap

        REQUIRE_INTERSECTION({{0.0, 0.0}, {2.0, 2.0}}, {{0.0, 2.0}, {2.0, 0.0}}, {{1.0, 1.0}});  // proper crossing
        REQUIRE_INTERSECTION({{0.0, 0.0}, {1.0, 1.0}}, {{0.0, 2.0}, {1.0, 1.0}}, {{1.0, 1.0}});  // crossing at corner
        REQUIRE_INTERSECTION({{0.0, 0.0}, {2.0, 2.0}}, {{0.0, 2.0}, {1.0, 1.0}}, {{1.0, 1.0}});  // crossing in middle
        REQUIRE_INTERSECTION({{0.0, 0.0}, {2.0, 2.0}}, {{0.0, 2.0}, {0.5, 1.5}}, std::nullopt);  // no crossing
    }

    MSC_AUTO_TEST_CASE(intersection_boolean_constexpr)
    {
        {
            constexpr auto p1 = msc::point2d{0.0, 0.0};
            constexpr auto p2 = msc::point2d{2.0, 1.0};
            constexpr auto p3 = msc::point2d{3.0, 1.5};
            constexpr auto p4 = msc::point2d{4.0, 2.0};
            static_assert(!msc::check_intersect(std::make_pair(p1, p2), std::make_pair(p3, p4)));
        }
        {
            constexpr auto p1 = msc::point2d{0.0, 0.0};
            constexpr auto p2 = msc::point2d{2.0, 1.0};
            constexpr auto p3 = msc::point2d{2.0, 1.0};
            constexpr auto p4 = msc::point2d{4.0, 2.0};
            static_assert(msc::check_intersect(std::make_pair(p1, p2), std::make_pair(p3, p4)));
        }
        {
            constexpr auto p1 = msc::point2d{1.0, 4.0};
            constexpr auto p2 = msc::point2d{3.0, 4.0};
            constexpr auto p3 = msc::point2d{2.0, 5.0};
            constexpr auto p4 = msc::point2d{5.0, 5.0};
            static_assert(!msc::check_intersect(std::make_pair(p1, p2), std::make_pair(p3, p4)));
        }
        {
            constexpr auto p1 = msc::point2d{1.0, 1.0};
            constexpr auto p2 = msc::point2d{3.0, 1.0};
            constexpr auto p3 = msc::point2d{2.0, 0.0};
            constexpr auto p4 = msc::point2d{2.0, 2.0};
            static_assert(msc::check_intersect(std::make_pair(p1, p2), std::make_pair(p3, p4)));
        }
        {
            constexpr auto p1 = msc::point2d{10.0, -10.0};
            constexpr auto p2 = msc::point2d{30.0, -10.0};
            constexpr auto p3 = msc::point2d{ 0.0, -20.0};
            constexpr auto p4 = msc::point2d{ 0.0,   0.0};
            static_assert(!msc::check_intersect(std::make_pair(p1, p2), std::make_pair(p3, p4)));
        }
    }

    int get_int_from_env(const char *const envvar, const int fallback)
    {
        using namespace std::string_literals;
        if (const auto envval = std::getenv(envvar)) {
            try {
                const auto value = std::stoi(envval);
                if (value >= 0) { return value; }
            } catch (const std::invalid_argument&) {
                MSC_FAIL("Environment variable "s + envvar + " must be set to a non-negative integer"s);
            }
        }
        return fallback;
    }

    MSC_AUTO_TEST_CASE(sanity_check_find_edge_crossings)
    {
        const auto debugout = std::getenv("MSC_TEST_SANITY_CHECK_EDGE_CROSSINGS_OUTPUT");
        const auto n = get_int_from_env("MSC_TEST_SANITY_CHECK_EDGE_CROSSINGS_NODES", 30);
        const auto m = get_int_from_env("MSC_TEST_SANITY_CHECK_EDGE_CROSSINGS_EDGES", 45);
        MSC_REQUIRE((n >= 0) && (m >= 0) && (n * (n - 1) / 2 >= m));
        const auto graph_color = ogdf::Color(0x72, 0x9F, 0xCF);
        const auto cross_color = ogdf::Color(0xCC, 0x00, 0x00);
        const auto [graph, attrs] = msc::test::make_test_layout(n, m, std::getenv("MSC_RANDOM_SEED"));
        const auto crossings = msc::find_edge_crossings(*attrs);
        attrs->addAttributes(ogdf::GraphAttributes::nodeStyle | ogdf::GraphAttributes::edgeStyle);
        for (const auto v : graph->nodes) {
            attrs->fillColor(v)   = graph_color;
            attrs->strokeColor(v) = graph_color;
            attrs->shape(v)       = ogdf::Shape::Ellipse;
            attrs->width(v)       = 5.0;
            attrs->height(v)      = 5.0;
        }
        for (const auto e : graph->edges) {
            attrs->strokeColor(e) = graph_color;
        }
        for (const auto [p, e1, e2] : crossings) {
            const auto v = graph->newNode();
            attrs->x(v)           = p.x();
            attrs->y(v)           = p.y();
            attrs->fillColor(v)   = cross_color;
            attrs->strokeColor(v) = cross_color;
            attrs->shape(v)       = ogdf::Shape::Rect;
            attrs->width(v)       = 2.0;
            attrs->height(v)      = 2.0;
            (void) e1;
            (void) e2;
        }
        const auto destination = msc::output_file{(debugout != nullptr) ? debugout : "NULL"};
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = msc::prepare_stream(stream, destination);
        if (!ogdf::GraphIO::drawSVG(*attrs, stream) || !stream.flush().good()) {
            MSC_FAIL("Cannot dump graph drawing with crossings marked to '" + name + "'");
        }
        MSC_SKIP_IF(debugout == nullptr);  // for the record
    }

}  // namespace /*anonymous*/
