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
#include <config.h>
#endif

#define MSC_RUN_ALL_UNIT_TESTS_IN_MAIN

#include "sliding.hxx"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

#include "math_constants.hxx"
#include "stochastic.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(gaussian_kernel_no_events)
    {
        const auto values = std::vector<double>{};
        const auto kernel = msc::gaussian_kernel{std::cbegin(values), std::cend(values), 14.92};
        MSC_REQUIRE_EQ(0.0, kernel(-60.2));
        MSC_REQUIRE_EQ(0.0, kernel(  0.0));
        MSC_REQUIRE_EQ(0.0, kernel(+77.7));
    }

    MSC_AUTO_TEST_CASE(gaussian_kernel_one_event)
    {
        const auto x = 14.92;
        const auto sigma = 3.0;
        const auto values = std::vector<double>{x};
        const auto kernel = msc::gaussian_kernel{std::cbegin(values), std::cend(values), sigma};
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::normal_distribution{x, sigma};
        for (auto i = 0; i < 10; ++i) {
            MSC_REQUIRE_GT(kernel(x), kernel(x + rnddst(rndeng)));
        }
    }

    MSC_AUTO_TEST_CASE(gaussian_kernel_two_events)
    {
        // TODO: Determine the actual maxima analytically and test for them?
        const auto x1 = 14.0;
        const auto x2 = 92.0;
        const auto sigma = std::abs(x1 - x2) / 3.0;
        const auto values = std::vector<double>{x1, x2};
        const auto kernel = msc::gaussian_kernel{std::cbegin(values), std::cend(values), sigma};
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution{std::min(x1, x2) + 0.1 * sigma, std::max(x1, x2) - 0.1 * sigma};
        MSC_REQUIRE_CLOSE(1.0E-10, kernel(x1), kernel(x2));
        MSC_REQUIRE_GT(kernel(x1), kernel((x1 + x2) / 2.0));
        MSC_REQUIRE_GT(kernel(x2), kernel((x1 + x2) / 2.0));
        for (auto i = 0; i < 10; ++i) {
            const auto x = rnddst(rndeng);
            MSC_REQUIRE_LE(kernel(x), kernel(x1));
            MSC_REQUIRE_LE(kernel(x), kernel(x2));
            MSC_REQUIRE_GE(kernel(x), kernel((x1 + x2) / 2.0));
        }
    }

    MSC_AUTO_TEST_CASE(make_density)
    {
        const auto func = [](const double x){ const auto y = std::sin(x); return y * y; };
        const auto xlo = -3.0;
        const auto xhi = +0.5;
        const auto points = 100;
        const auto density = msc::make_density(func, xlo, xhi, points, false);
        MSC_REQUIRE_EQ(points, density.size());
        MSC_REQUIRE_EQ(xlo, density.front().first);
        MSC_REQUIRE_EQ(xhi, density.back().first);
        for (const auto [x, y] : density) {
            MSC_REQUIRE_CLOSE(1.0E-20, y, func(x));
        }
        for (auto i = 1; i < points; ++i) {
            MSC_REQUIRE_CLOSE(1.0E-10, density[i].first - density[i - 1].first, (xhi - xlo) / (points - 1));
        }
    }

    MSC_AUTO_TEST_CASE(make_density_normalized)
    {
        constexpr auto a = 4.0;
        constexpr auto b = 0.3;
        const auto func = [](const double x){ return a + b * x; };
        const auto xlo = -1.0;
        const auto xhi = +3.0;
        const auto points = 100;
        const auto density = msc::make_density(func, xlo, xhi, points);  // normalizing ought to be the default
        MSC_REQUIRE_EQ(points, density.size());
        MSC_REQUIRE_EQ(xlo, density.front().first);
        MSC_REQUIRE_EQ(xhi, density.back().first);
        const auto area = a * (xhi - xlo) + 0.5 * b * (xhi * xhi - xlo * xlo);
        for (const auto [x, y] : density) {
            MSC_REQUIRE_CLOSE(1.0E-10, y, func(x) / area);
        }
        for (auto i = 1; i < points; ++i) {
            MSC_REQUIRE_CLOSE(1.0E-10, density[i].first - density[i - 1].first, (xhi - xlo) / (points - 1));
        }
    }

    MSC_AUTO_TEST_CASE(make_density_adaptive)
    {
        const auto func = [](const double x){ return std::exp(-(x * x)); };
        const auto xlo = -3.0;
        const auto xhi = +3.0;
        const auto density = msc::make_density_adaptive(func, xlo, xhi, false);
        MSC_REQUIRE_GE(density.size(), 10);
        MSC_REQUIRE_EQ(xlo, density.front().first);
        MSC_REQUIRE_EQ(xhi, density.back().first);
        for (const auto [x, y] : density) {
            MSC_REQUIRE_CLOSE(1.0E-20, y, func(x));
        }
        MSC_REQUIRE(std::is_sorted(std::begin(density), std::end(density)));  // default comparison is fine
    }

    MSC_AUTO_TEST_CASE(make_density_adaptive_normalized)
    {
        const auto func = [](const double x){ return 1.0 + std::sin(x); };
        const auto xlo = 0.0;
        const auto xhi = 2.0 * M_PI;
        const auto density = msc::make_density_adaptive(func, xlo, xhi);  // normalizing ought to be the default
        MSC_REQUIRE_GE(density.size(), 10);
        MSC_REQUIRE_EQ(xlo, density.front().first);
        MSC_REQUIRE_EQ(xhi, density.back().first);
        const auto area = 1.0 * (xhi - xlo);
        for (const auto [x, y] : density) {
            MSC_REQUIRE_CLOSE(1.0E-3, y, func(x) / area);
        }
        MSC_REQUIRE(std::is_sorted(std::begin(density), std::end(density)));  // default comparison is fine
    }

    // https://de.wikipedia.org/wiki/Differentielle_Entropie#Differentielle_Entropie_f%C3%BCr_verschiedene_Verteilungen

    MSC_AUTO_TEST_CASE(get_differential_entropy_of_pdf_uniform_distribution)
    {
        for (auto width = 0.5; width < 3.0; width += 0.5) {
            const auto mean = -13.5;
            auto samples = std::vector<std::pair<double, double>>{};
            for (auto x = mean - width; x < mean + width; x += 0.01 * width) {
                const auto y = ((x >= mean - width / 2.0) && (x <= mean + width / 2.0)) ? 1.0 / width : 0.0;
                samples.emplace_back(x, y);
            }
            MSC_REQUIRE_LT(samples.front().second, 1.0E-10);
            MSC_REQUIRE_LT(samples.back().second, 1.0E-10);
            const auto expected = 0.5 * std::log2(width * width);
            const auto actual = msc::get_differential_entropy_of_pdf(samples);
            MSC_REQUIRE_CLOSE(1.0E-10, expected, actual);
        }
    }

    MSC_AUTO_TEST_CASE(get_differential_entropy_of_pdf_normal_distribution)
    {
        for (auto sigma = 0.125; sigma < 8.0; sigma *= 2.0) {
            const auto mu = 42.0;
            const auto gaussian = msc::gaussian{mu, sigma};
            auto samples = std::vector<std::pair<double, double>>{};
            for (auto x = mu - 10.0 * sigma; x <= mu + 10.0 * sigma; x += 0.1 * sigma) {
                samples.emplace_back(x, gaussian(x));
            }
            MSC_REQUIRE_LT(samples.front().second, 1.0E-10);
            MSC_REQUIRE_LT(samples.back().second, 1.0E-10);
            const auto expected = 0.5 * std::log2(2.0 * M_PI * M_E * sigma * sigma);
            const auto actual = msc::get_differential_entropy_of_pdf(samples);
            MSC_REQUIRE_CLOSE(1.0E-10, 1.0, actual / expected);
        }
    }

}  // namespace /*anonymous*/
