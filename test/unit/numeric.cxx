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

#include "numeric.hxx"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

#include "unittest.hxx"

namespace /*anonymous*/
{

    using vec2f = std::vector<std::pair<double, double>>;

    template <typename EngT, typename F1T, typename F2T>
    void generic_integral_test(EngT& rndeng, F1T&& f, F2T&& F, const std::size_t n = 100, const double tol = 1.0E-2)
    {
        auto rnddst = std::uniform_real_distribution{-10.0, +10.0};
        auto points = vec2f(n);
        const auto lambda = [f, &rndeng, &rnddst](){
            const auto x = rnddst(rndeng);
            return std::make_pair(x, f(x));
        };
        const auto compare = [](const auto& lhs, const auto& rhs){
            return lhs.first < rhs.first;
        };
        std::generate(std::begin(points), std::end(points), lambda);
        std::sort(std::begin(points), std::end(points), compare);
        const auto lo = points.front().first;
        const auto hi = points.back().first;
        const auto expected = F(hi) - F(lo);
        const auto actual = msc::integrate_trapezoidal(std::begin(points), std::end(points));
        MSC_REQUIRE_CLOSE(tol, expected, actual);
    }

    MSC_AUTO_TEST_CASE(integrate_zero)
    {
        const auto f = [](const double /*x*/){ return 0.0; };
        const auto F = [](const double /*x*/){ return 0.0; };
        auto rndeng = std::default_random_engine{};
        for (const auto n : {2, 3, 10, 100}) {
            generic_integral_test(rndeng, f, F, n, 1.0E-10);
        }
    }

    MSC_AUTO_TEST_CASE(integrate_constant)
    {
        constexpr auto c = 17.8;
        const auto f = [](const double /*x*/){ return c; };
        const auto F = [](const double x){ return c * x; };
        auto rndeng = std::default_random_engine{};
        for (const auto n : {2, 3, 10, 100}) {
            generic_integral_test(rndeng, f, F, n, 1.0E-10);
        }
    }

    MSC_AUTO_TEST_CASE(integrate_linear)
    {
        constexpr auto d = 4.2;
        constexpr auto k = -2.8;
        const auto f = [](const double x){ return d + k * x; };
        const auto F = [](const double x){ return d * x + k * std::pow(x, 2.0) / 2.0; };
        auto rndeng = std::default_random_engine{};
        for (const auto n : {2, 3, 10, 100}) {
            generic_integral_test(rndeng, f, F, n, 1.0E-10);
        }
    }

    MSC_AUTO_TEST_CASE(integrate_quadratic)
    {
        constexpr auto a = 3.4;
        constexpr auto b = -0.05;
        constexpr auto c = 0.003;
        const auto f = [](const double x){ return a + b * x + c * std::pow(x, 2.0); };
        const auto F = [](const double x){ return a * x + b * std::pow(x, 2.0) / 2.0 + c * std::pow(x, 3.0) / 3.0; };
        auto rndeng = std::default_random_engine{};
        generic_integral_test(rndeng, f, F);
    }

    MSC_AUTO_TEST_CASE(integrate_sinus)
    {
        const auto f = [](const double x){ return std::sin(x); };
        const auto F = [](const double x){ return std::cos(x); };
        auto rndeng = std::default_random_engine{};
        generic_integral_test(rndeng, f, F);
    }

}  // namespace /*anonymous*/
