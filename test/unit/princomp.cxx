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

#include "princomp.hxx"

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

#include "point.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(pca_in_null_vector_space_is_pointless_but_compiles)
    {
        using point_type = msc::point<double, 0>;
        constexpr auto tol = 1.0E-10;
        auto rndeng = std::default_random_engine{};
        auto pop = std::vector<point_type>(10);
        const auto primary = msc::find_primary_axes(pop, rndeng);
        MSC_REQUIRE(std::all_of(std::begin(pop), std::end(pop), [](const point_type& p){ return abs(p) <= tol; }));
        MSC_REQUIRE_EQ(0, primary.size());
    }

    MSC_AUTO_TEST_CASE(one_dimensional_normal_data)
    {
        using point_type = msc::point<double, 1>;
        constexpr auto tol = 1.0E-10;
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::normal_distribution{42.0, 13.5};
        auto pop = std::vector<point_type>(30);
        std::generate(std::begin(pop), std::end(pop), [&rndeng, &rnddst](){ return rnddst(rndeng); });
        const auto [pc] = msc::find_primary_axes(pop, rndeng);
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc));
        MSC_REQUIRE(std::all_of(std::begin(pop), std::end(pop), [](const point_type& p){ return abs(p) <= tol; }));
    }

    MSC_AUTO_TEST_CASE(two_dimensional_normal_data)
    {
        using point_type = msc::point<double, 2>;
        constexpr auto tol = 1.0E-10;
        auto rndeng = std::default_random_engine{};
        auto rnddst0 = std::normal_distribution{100.0,  5.0};
        auto rnddst1 = std::normal_distribution{-50.0, 70.0};
        auto pop = std::vector<point_type>(1000);
        const auto pointgen = [&rndeng, &rnddst0, &rnddst1]()->point_type{
            return {rnddst0(rndeng), rnddst1(rndeng)};
        };
        std::generate(std::begin(pop), std::end(pop), pointgen);
        const auto [pc0, pc1] = msc::find_primary_axes(pop, rndeng);
        MSC_REQUIRE(std::all_of(std::begin(pop), std::end(pop), [](const point_type& p){ return abs(p) <= tol; }));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc0));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc1));
        MSC_REQUIRE_LE(distance(pc0, point_type{0.0, 1.0}), 1.0E-2);
        MSC_REQUIRE_LE(distance(pc1, point_type{1.0, 0.0}), 1.0E-2);
    }

    MSC_AUTO_TEST_CASE(two_dimensional_normal_data_rotated)
    {
        using point_type = msc::point<double, 2>;
        constexpr auto tol = 1.0E-10;
        const auto normalized = [](const point_type& p){ return p / abs(p); };
        const auto offset = point_type{12.3, -4.5};
        const auto major = point_type{5.0, 5.0};
        const auto minor = point_type{-1.0, 1.0};
        MSC_REQUIRE_CLOSE(tol, 0.0, dot(major, minor));
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::normal_distribution{};
        auto pop = std::vector<point_type>(1000);
        const auto pointgen = [offset, major, minor, &rndeng, &rnddst]()->point_type{
            const auto p = msc::make_random_point<double, 2>(rndeng, rnddst);
            return offset + major * dot(major, p) + minor * dot(minor, p);
        };
        std::generate(std::begin(pop), std::end(pop), pointgen);
        const auto [pc0, pc1] = msc::find_primary_axes(pop, rndeng);
        MSC_REQUIRE(std::all_of(std::begin(pop), std::end(pop), [](const point_type& p){ return abs(p) <= tol; }));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc0));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc1));
        MSC_REQUIRE_LE(distance(pc0, normalized(point_type{+1.0, +1.0})), 1.0E-2);
        MSC_REQUIRE_LE(distance(pc1, normalized(point_type{-1.0, +1.0})), 1.0E-2);
    }

    MSC_AUTO_TEST_CASE(high_dimensional_normal_data)
    {
        constexpr auto N = std::integral_constant<std::size_t, 7>{};
        constexpr auto M = std::integral_constant<std::size_t, 3>{};
        using point_type = msc::point<double, N>;
        const auto normalized = [](const point_type& p){ return p / abs(p); };
        constexpr auto tol = 1.0E-10;
        const auto weights = normalized({0.0, 10.0, 0.2, 0.3, 4.0, -0.05, 0.6});
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::normal_distribution{};
        auto pop = std::vector<point_type>(10000);
        const auto pointgen = [weights, &rndeng, &rnddst]()->point_type{
            // For some reason, my compiler doesn't see the constexpr through the lambda.
            auto p = msc::make_random_point<double, decltype(N){}>(rndeng, rnddst);
            for (std::size_t i = 0; i < decltype(N){}; ++i) {
                p[i] *= weights[i];
            }
            return p;
        };
        std::generate(std::begin(pop), std::end(pop), pointgen);
        const auto [pc0, pc1, pc2] = msc::find_primary_axes(pop, rndeng, M);
        MSC_REQUIRE(!std::all_of(std::begin(pop), std::end(pop), [](const point_type& p){ return abs(p) <= tol; }));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc0));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc1));
        MSC_REQUIRE_CLOSE(tol, 1.0, abs(pc2));
        MSC_REQUIRE_LE(distance(pc0, msc::make_unit_point<double, N>(1)), 1.0E-2);
        MSC_REQUIRE_LE(distance(pc1, msc::make_unit_point<double, N>(4)), 1.0E-2);
        MSC_REQUIRE_LE(distance(pc2, msc::make_unit_point<double, N>(6)), 1.0E-2);
    }

}  // namespace /*anonymous*/
