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

#include "stochastic.hxx"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

#include "math_constants.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(square)
    {
        static_assert(0 == msc::square(0));
        static_assert(1.0 == msc::square(1.0));
        static_assert(4.0 == msc::square(2.0));
        static_assert(529 == msc::square(23));
    }

    MSC_AUTO_TEST_CASE(mean_stdev_1st)
    {
        constexpr auto tol = 1.0e-15;
        const std::array<double, 3> numbers = {{42.0, 42.0, 42.0}};
        const auto [mean, stdev] = msc::mean_stdev(numbers);
        MSC_REQUIRE_CLOSE(tol, 42.0, mean);
        MSC_REQUIRE_CLOSE(tol, 0.0, stdev);
    }

    MSC_AUTO_TEST_CASE(mean_stdev_2nd)
    {
        constexpr auto tol = 1.0e-10;
        constexpr auto n = std::size_t{100};
        const auto numbers = [](){
            auto vec = std::vector<double>(n);
            std::iota(std::begin(vec), std::end(vec), 1.0);
            return vec;
        }();
        MSC_REQUIRE_EQ(1.0, numbers.front());
        MSC_REQUIRE_EQ(100.0, numbers.back());
        const auto [mean, stdev] = msc::mean_stdev(numbers);
        MSC_REQUIRE_CLOSE(tol, 50.500000000000000, mean);
        MSC_REQUIRE_CLOSE(tol, 29.011491975882000, stdev);
    }

    MSC_AUTO_TEST_CASE(mean_stdev_3rd)
    {
        constexpr auto reltol = 1.0e-3;
        constexpr auto n = std::size_t{600000};
        auto rndeng = std::mt19937{};
        auto metadist = std::uniform_real_distribution<double>{1.0, 2.0};
        auto rnddst = std::normal_distribution<double>{100.0 * metadist(rndeng), 3.0 * metadist(rndeng)};
        auto numbers = std::vector<double>(n);
        std::generate(std::begin(numbers), std::end(numbers), [&rndeng, &rnddst](){ return rnddst(rndeng); });
        const auto [mean, stdev] = msc::mean_stdev(numbers);
        MSC_REQUIRE_CLOSE(reltol, 1.0, rnddst.mean() / mean);
        MSC_REQUIRE_CLOSE(reltol, 1.0, rnddst.stddev() / stdev);
    }

    MSC_AUTO_TEST_CASE(min_max_1)
    {
        const auto numbers = std::initializer_list<int>{1};
        const auto [min, max] = msc::min_max(numbers);
        MSC_REQUIRE_EQ(1, min);
        MSC_REQUIRE_EQ(1, max);
    }

    MSC_AUTO_TEST_CASE(min_max_2)
    {
        const auto numbers = std::initializer_list<int>{7, 12};
        const auto [min, max] = msc::min_max(numbers);
        MSC_REQUIRE_EQ(7, min);
        MSC_REQUIRE_EQ(12, max);
    }

    MSC_AUTO_TEST_CASE(min_max_3)
    {
        const auto numbers = std::initializer_list<int>{1, 2, 3};
        const auto [min, max] = msc::min_max(numbers);
        MSC_REQUIRE_EQ(1, min);
        MSC_REQUIRE_EQ(3, max);
    }

    MSC_AUTO_TEST_CASE(min_max_n)
    {
        constexpr auto n = std::size_t{1000};
        const auto numbers = [](){
            auto vec = std::vector<double>(n);
            auto rndeng = std::mt19937{};
            auto rnddst = std::uniform_real_distribution<double>{14.0, 92.0};
            std::generate(std::begin(vec), std::end(vec), [&rndeng, &rnddst](){ return rnddst(rndeng); });
            return vec;
        }();
        const auto [min, max] = msc::min_max(numbers);
        MSC_REQUIRE_LE(min, max);
        MSC_REQUIRE(std::all_of(std::begin(numbers), std::end(numbers), [min](const double x){ return (min <= x); }));
        MSC_REQUIRE(std::all_of(std::begin(numbers), std::end(numbers), [max](const double x){ return (max >= x); }));
        MSC_REQUIRE(std::any_of(std::begin(numbers), std::end(numbers), [min](const double x){ return (min == x); }));
        MSC_REQUIRE(std::any_of(std::begin(numbers), std::end(numbers), [max](const double x){ return (max == x); }));
    }

    MSC_AUTO_TEST_CASE(entropy_of_nothing_is_zero)
    {
        const auto data = std::vector<double>{};
        const auto entropy = msc::entropy(data);
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, entropy);
    }

    MSC_AUTO_TEST_CASE(entropy_of_one_thing_is_zero)
    {
        const auto data = std::vector<double>{ 1.0 };
        const auto entropy = msc::entropy(data);
        MSC_REQUIRE_CLOSE(1.0E-10, 0.0, entropy);
    }

    MSC_AUTO_TEST_CASE(entropy_of_two_equal_things_is_one)
    {
        const auto data = std::vector<double>{ 0.5, 0.5 };
        const auto entropy = msc::entropy(data);
        MSC_REQUIRE_CLOSE(1.0E-10, 1.0, entropy);
    }

    MSC_AUTO_TEST_CASE(entropy_of_two_unequal_things_is_between_zero_and_one)
    {
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution{0.0, 1.0};
        for (auto i = 0; i < 10; ++i) {
            const auto x = rnddst(rndeng);
            const auto data = std::vector<double>{ x, 1.0 - x };
            const auto entropy = msc::entropy(data);
            MSC_REQUIRE_GT(entropy, 0.0);
            MSC_REQUIRE_LT(entropy, 1.0);
        }
    }

    MSC_AUTO_TEST_CASE(gaussian_standard)
    {
        const auto g = msc::gaussian{};
        MSC_REQUIRE_CLOSE(1.0E-20, 1.0 / std::sqrt(2.0 * M_PI), g(0.0));
        MSC_REQUIRE_CLOSE(1.0E-20, 1.0 / std::sqrt(2.0 * M_PI * M_E), g(1.0));
    }

    MSC_AUTO_TEST_CASE(gaussian_nonstandard)
    {
        const auto sqr = [](auto x){ return x * x; };
        auto rndeng = std::default_random_engine{};
        auto rnddst_mu = std::uniform_real_distribution{-10.0, +10.0};
        auto rnddst_sigma = std::uniform_real_distribution{0.01, 10.0};
        for (auto i = 0; i < 100; ++i) {
            const auto mu = rnddst_mu(rndeng);
            const auto sigma = rnddst_sigma(rndeng);
            const auto g = msc::gaussian{mu, sigma};
            MSC_REQUIRE_CLOSE(1.0E-10, 1.0 / (sigma * std::sqrt(2.0 * M_PI)), g(mu));
            MSC_REQUIRE_CLOSE(1.0E-10, 1.0 / (sigma * std::sqrt(2.0 * M_PI * M_E)), g(mu + sigma));
            MSC_REQUIRE_CLOSE(1.0E-10, 1.0 / (sigma * std::sqrt(2.0 * M_PI * M_E)), g(mu - sigma));
            auto normdst = std::normal_distribution{mu, sigma};
            for (auto j = 0; j < 100; ++j) {
                const auto x = normdst(rndeng);
                const auto expected = std::exp(-0.5 * sqr((x - mu) / sigma)) / (sigma * std::sqrt(2.0 * M_PI));
                const auto actual = g(x);
                MSC_REQUIRE_CLOSE(1.0E-10, expected, actual);
            }
        }
    }

}  // namespace /*anonymous*/
