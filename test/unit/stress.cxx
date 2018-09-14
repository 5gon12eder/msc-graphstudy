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

#include "stress.hxx"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "normalizer.hxx"
#include "testaux/cube.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(no_nodes)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        MSC_REQUIRE_EQ(0.0, msc::compute_stress(*attrs, 27.0));
        MSC_REQUIRE_EQ(0.0, msc::compute_stress_fit_nodesep(*attrs).y0);
        MSC_REQUIRE_EQ(0.0, msc::compute_stress_fit_scale(*attrs).y0);
    }

    MSC_AUTO_TEST_CASE(no_edges)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        for (auto i = 0; i < 10; ++i) {
            graph->newNode();
        }
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        MSC_REQUIRE_EQ(0.0, msc::compute_stress(*attrs, 27.0));
        MSC_REQUIRE_EQ(0.0, msc::compute_stress_fit_nodesep(*attrs).y0);
        MSC_REQUIRE_EQ(0.0, msc::compute_stress_fit_scale(*attrs).y0);
    }

    MSC_AUTO_TEST_CASE(single_edge)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->x(v1) = -10.0;
        attrs->y(v1) = -10.0;
        attrs->x(v2) = +70.0;
        attrs->y(v2) = +50.0;
        MSC_REQUIRE_GT(msc::compute_stress(*attrs, 27.0), 0.0);
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, msc::compute_stress_fit_nodesep(*attrs).y0);
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, msc::compute_stress_fit_scale(*attrs).y0);
    }

    MSC_AUTO_TEST_CASE(perfect_layout)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution{10.0, 1000.0};
        for (auto i = 0; i < 10; ++i) {
            const auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
            attrs->x(v1) = 1.0;  attrs->y(v1) = 0.0;
            attrs->x(v2) = 2.0;  attrs->y(v2) = 0.0;
            attrs->x(v3) = 3.0;  attrs->y(v3) = 0.0;
            attrs->scale(rnddst(rndeng));
            MSC_REQUIRE_CLOSE(1.0E-2, 0.0, msc::compute_stress_fit_nodesep(*attrs).y0);
            MSC_REQUIRE_CLOSE(1.0E-2, 0.0, msc::compute_stress_fit_scale(*attrs).y0);
        }
    }

    MSC_AUTO_TEST_CASE(imperfect_layout)
    {
        const auto [graph, attrs] = msc::test::make_test_layout(42, 100);
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution{0.5, 2.0};
        for (auto i = 0; i < 10; ++i) {
            msc::normalize_layout(*attrs);
            attrs->scale(rnddst(rndeng));
            const auto result = msc::compute_stress_fit_nodesep(*attrs);
            MSC_REQUIRE_GT(result.x0, 0.0);
            MSC_REQUIRE_GT(result.y0, 0.0);
            MSC_REQUIRE_GT(msc::compute_stress_fit_scale(*attrs).y0, 0.0);
        }
    }

    MSC_AUTO_TEST_CASE(sanity_check_fit_nodesep)
    {
        using namespace std::string_literals;
        const auto debugdir = std::getenv("MSC_TEST_PARABOLA_DEBUGDIR");
        MSC_SKIP_UNLESS(debugdir != nullptr);
        const auto [graph, attrs] = msc::test::make_test_layout(42, 100, std::getenv("MSC_RANDOM_SEED"));
        const auto result = msc::compute_stress_fit_nodesep(*attrs);
        const auto xmin = 0.5 * result.x0;
        const auto xmax = 1.5 * result.x0;
        const auto step = (xmax - xmin) / 10.0;
        {
            auto ostr = std::ofstream{debugdir + "/stress-fit-nodesep.gplt"s};
            ostr << std::setprecision(10) << std::scientific;
            ostr << "#! /usr/bin/gnuplot\n";
            ostr << "\n";
            ostr << "set terminal svg size 800,600 noenhance\n";
            ostr << "set output 'stress-fit-nodesep.svg'\n";
            ostr << "regression(x) = " << result.a << " + " << result.b << " * x + " << result.c << "* x**2\n";
            ostr << "set xrange [" << xmin - step << " : " << xmax + step << "]\n";
            ostr << "set title 'STRESS_FIT_NODESEP'\n";
            ostr << "set xlabel 'node distance'\n";
            ostr << "set ylabel 'stress'\n";
            ostr << "set label at " << result.x0 << ", " << result.y0 << " '' point pt 6 lc '#cc0000'\n";
            ostr << "plot 'stress-fit-nodesep.txt' title 'stress' linecolor '#73d216', \\\n"
                 << "\tregression(x) title 'regression' linecolor '#3465a4'\n";
        }
        {
            auto ostr = std::ofstream{debugdir + "/stress-fit-nodesep.txt"s};
            ostr << std::setprecision(10) << std::scientific;
            for (auto x = xmin; x <= xmax; x += step) {
                ostr << x << " " << msc::compute_stress(*attrs, x) << "\n";
            }
        }
    }

    MSC_AUTO_TEST_CASE(sanity_check_fit_scale)
    {
        using namespace std::string_literals;
        const auto debugdir = std::getenv("MSC_TEST_PARABOLA_DEBUGDIR");
        MSC_SKIP_UNLESS(debugdir != nullptr);
        const auto [graph, attrs] = msc::test::make_test_layout(42, 100, std::getenv("MSC_RANDOM_SEED"));
        const auto result = msc::compute_stress_fit_scale(*attrs);
        const auto xmin = 0.5 * result.x0;
        const auto xmax = 1.5 * result.x0;
        const auto step = (xmax - xmin) / 10.0;
        {
            auto ostr = std::ofstream{debugdir + "/stress-fit-scale.gplt"s};
            ostr << std::setprecision(10) << std::scientific;
            ostr << "#! /usr/bin/gnuplot\n";
            ostr << "\n";
            ostr << "set terminal svg size 800,600 noenhance\n";
            ostr << "set output 'stress-fit-scale.svg'\n";
            ostr << "regression(x) = " << result.a << " + " << result.b << " * x + " << result.c << "* x**2\n";
            ostr << "set xrange [" << xmin - step << " : " << xmax + step << "]\n";
            ostr << "set title 'STRESS_FIT_SCALE'\n";
            ostr << "set xlabel 'layout scale'\n";
            ostr << "set ylabel 'stress'\n";
            ostr << "set label at " << result.x0 << ", " << result.y0 << " '' point pt 6 lc '#cc0000'\n";
            ostr << "plot 'stress-fit-scale.txt' title 'stress' linecolor '#73d216', \\\n"
                 << "\tregression(x) title 'regression' linecolor '#3465a4'\n";
        }
        {
            auto ostr = std::ofstream{debugdir + "/stress-fit-scale.txt"s};
            ostr << std::setprecision(10) << std::scientific;
            for (auto x = xmin; x <= xmax; x += step) {
                const auto copy = std::make_unique<ogdf::GraphAttributes>(*attrs);
                copy->scale(x);
                ostr << x << " " << msc::compute_stress(*copy) << "\n";
            }
        }
    }

}  // namespace /*anonymous*/
