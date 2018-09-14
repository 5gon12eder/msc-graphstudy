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

#include "regression.hxx"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

#include "math_constants.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(no_data_gives_nans)
    {
        const auto data = std::vector<std::pair<float, int>>{};
        const auto [d, k] = msc::linear_regression(data);
        static_assert(std::is_same_v<decltype(d), const float>);
        static_assert(std::is_same_v<decltype(k), const float>);
        MSC_REQUIRE(std::isnan(d));
        MSC_REQUIRE(std::isnan(k));
    }

    MSC_AUTO_TEST_CASE(single_value_gives_constant)
    {
        const auto tol = 1.0E-10;
        const auto data = std::vector<std::array<double, 2>>{{5.0, 42.0}};
        const auto [d, k] = msc::linear_regression(data);
        MSC_REQUIRE_CLOSE(tol, 42.0, d);
        MSC_REQUIRE_CLOSE(tol,  0.0, k);
    }

    MSC_AUTO_TEST_CASE(degenerated_values_give_arithmetic_mean)
    {
        const auto tol = 1.0E-10;
        const auto data = std::vector<std::array<double, 2>>{{0.0, 1.0}, {0.0, 2.0}, {0.0, 3.0}};
        const auto [d, k] = msc::linear_regression(data);
        MSC_REQUIRE_CLOSE(tol, 2.0, d);
        MSC_REQUIRE_CLOSE(tol, 0.0, k);
    }

    MSC_AUTO_TEST_CASE(linear_function_is_recovered)
    {
        const auto tol = 1.0E-10;
        const auto d = -1.4;
        const auto k = -9.2;
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution<double>{-100.0, +100.0};
        auto data = std::vector<std::array<double, 2>>{};
        std::generate_n(
            std::back_inserter(data), 100, [d, k, &rndeng, &rnddst]()->std::array<double, 2>{
                const auto x = rnddst(rndeng);
                const auto y = d + k * x;
                return {x, y};
            });
        const auto regression = msc::linear_regression(data);
        MSC_REQUIRE_CLOSE(tol, d, std::get<0>(regression));
        MSC_REQUIRE_CLOSE(tol, k, std::get<1>(regression));
    }

    MSC_AUTO_TEST_CASE(disturbed_linear_function_is_recovered)
    {
        const auto tol = 5.0E-2;
        const auto d = M_PI;
        const auto k = M_E;
        auto engine = std::default_random_engine{};
        auto xdist = std::uniform_real_distribution<double>{-10.0, +10.0};
        auto edist = std::normal_distribution<double>{0.0, 1.0};
        auto data = std::vector<std::array<double, 2>>{};
        std::generate_n(
            std::back_inserter(data), 100, [d, k, &engine, &xdist, &edist]()->std::array<double, 2>{
                const auto x = xdist(engine);
                const auto y = d + k * x;
                const auto delta = edist(engine);
                return {x, y + delta};
            });
        const auto regression = msc::linear_regression(data);
        MSC_REQUIRE_CLOSE(tol, 1.0, std::abs(std::get<0>(regression) / d));
        MSC_REQUIRE_CLOSE(tol, 1.0, std::abs(std::get<1>(regression) / k));
    }

}  // namespace /*anonymous*/
