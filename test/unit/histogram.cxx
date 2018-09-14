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

#include "histogram.hxx"

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(binnings_name)
    {
        MSC_REQUIRE_EQ("fixed-width", name(msc::binnings::fixed_width));
        MSC_REQUIRE_EQ("fixed-count", name(msc::binnings::fixed_count));
        MSC_REQUIRE_EQ("scott-normal-reference", name(msc::binnings::scott_normal_reference));
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, name(msc::binnings{}));
    }

    MSC_AUTO_TEST_CASE(simple)
    {
        const auto sqr = [](const auto x){ return x * x; };
        const auto data = std::vector<double>{-1.0, 2.0, -3.0, 4.0, -5.0};
        const auto histo = msc::histogram{data};
        MSC_REQUIRE_EQ(5, histo.size());
        MSC_REQUIRE_EQ(-5.0, histo.min());
        MSC_REQUIRE_EQ(+4.0, histo.max());
        MSC_REQUIRE_CLOSE(1.0e-10, (-1.0 + 2.0 - 3.0 + 4.0 - 5.0) / 5, histo.mean());
        MSC_REQUIRE_CLOSE(1.0e-10, std::sqrt((sqr(1.0) + sqr(2.0) + sqr(3.0) + sqr(4.0) + sqr(5.0)) / 5), histo.rms());
        MSC_REQUIRE_GT(histo.binwidth(), 0.0);
        MSC_REQUIRE_CLOSE(1.0e-10, 1.0, std::accumulate(histo.frequencies().begin(), histo.frequencies().end(), 0.0));
        MSC_REQUIRE_GT(histo.entropy(), 0.0);
    }

    MSC_AUTO_TEST_CASE(iota)
    {
        const auto data = [](){
            auto vec = std::vector<double>(100);
            std::iota(std::begin(vec), std::end(vec), 1.0);
            return vec;
        }();
        const auto histo = msc::histogram{data};
        MSC_REQUIRE_EQ(100, histo.size());
        MSC_REQUIRE_EQ(1.0, histo.min());
        MSC_REQUIRE_EQ(100.0, histo.max());
        MSC_REQUIRE_CLOSE(1.0e-10, 50.5000000000, histo.mean());
        MSC_REQUIRE_CLOSE(1.0e-10, 58.1678605417, histo.rms());
        MSC_REQUIRE_GT(histo.bincount(), std::size_t{1});
        MSC_REQUIRE_GT(histo.binwidth(), 0.0);
        MSC_REQUIRE_CLOSE(1.0e-10, 1.0, std::accumulate(histo.frequencies().begin(), histo.frequencies().end(), 0.0));
        MSC_REQUIRE_GT(histo.entropy(), 0.0);
    }

    MSC_AUTO_TEST_CASE(degenerate)
    {
        const auto data = std::vector<double>{7.0, 7.0, 7.0};
        const auto histo = msc::histogram{data};
        MSC_REQUIRE_EQ(3, histo.size());
        MSC_REQUIRE_EQ(7.0, histo.min());
        MSC_REQUIRE_EQ(7.0, histo.max());
        MSC_REQUIRE_CLOSE(1.0e-15, 7.0, histo.mean());
        MSC_REQUIRE_EQ(1, histo.bincount());
        MSC_REQUIRE_EQ(1.0, histo.binwidth());
        MSC_REQUIRE_CLOSE(1.0e-10, 1.0, std::accumulate(histo.frequencies().begin(), histo.frequencies().end(), 0.0));
        MSC_REQUIRE_CLOSE(1.0e-10, 7.0, histo.center(0));
        MSC_REQUIRE_CLOSE(1.0e-10, 1.0, histo.frequency(0));
        MSC_REQUIRE_CLOSE(1.0e-10, histo.entropy(), 0.0);
    }

    template <typename FuncT>
    void test_uniform_generic(FuncT&& factory)
    {
        constexpr auto n = std::size_t{1'000'000};
        const auto data = [](){
            auto rndeng = std::mt19937{};
            auto rnddst = std::uniform_real_distribution<double>{0.0, 1.0};
            auto vec = std::vector<double>{};
            vec.reserve(n);
            std::generate_n(std::back_inserter(vec), n, [&rndeng, &rnddst](){ return rnddst(rndeng); });
            return vec;
        }();
        const auto histo = factory(data);
        MSC_REQUIRE_EQ(n, histo.size());
        MSC_REQUIRE_CLOSE(1.0e-3, 0.0, histo.min());
        MSC_REQUIRE_CLOSE(1.0e-3, 1.0, histo.max());
        MSC_REQUIRE_CLOSE(1.0e-3, 0.5, histo.mean());
        MSC_REQUIRE_NE(histo.mean(), histo.rms());
        MSC_REQUIRE_GT(histo.bincount(), 1);
        MSC_REQUIRE_GT(histo.binwidth(), 0.0);
        MSC_REQUIRE_CLOSE(1.0e-10, 1.0, std::accumulate(histo.frequencies().begin(), histo.frequencies().end(), 0.0));
        MSC_REQUIRE_GT(histo.entropy(), 0.0);
        MSC_REQUIRE_CLOSE(1.0e-2, 0.5 / (histo.bincount() - 1), histo.frequencies().front());
        MSC_REQUIRE_CLOSE(1.0e-2, 0.5 / (histo.bincount() - 1), histo.frequencies().back());
        for (std::size_t i = 1; i + 1 < histo.bincount(); ++i) {
            MSC_REQUIRE_CLOSE(1.0e-2, 1.0 / (histo.bincount() - 1), histo.frequency(i));
        }
    }

    MSC_AUTO_TEST_CASE(uniform)
    {
        const auto factory = [](const auto& data){ return msc::histogram{data}; };
        test_uniform_generic(factory);
    }

    MSC_AUTO_TEST_CASE(uniform_bincount)
    {
        const auto factory = [](const auto& data){ return msc::histogram{data, std::size_t{42}}; };
        test_uniform_generic(factory);
    }

    MSC_AUTO_TEST_CASE(uniform_binwidth)
    {
        const auto factory = [](const auto& data){ return msc::histogram{data, 0.125}; };
        test_uniform_generic(factory);
    }

    MSC_AUTO_TEST_CASE(explicit_bincount)
    {
        const auto data = std::vector<double>{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        const auto histo = msc::histogram{data, std::size_t{4}};
        MSC_REQUIRE_EQ(12, histo.size());
        MSC_REQUIRE_EQ(4, histo.bincount());
        MSC_REQUIRE_EQ(28, histo.min());
        MSC_REQUIRE_EQ(31, histo.max());
        // Expected intervals: [27.5, 28.5), [28.5, 29.5), [29.5, 30.5), [30.5, 31.5)
        MSC_REQUIRE_EQ(28.0, histo.center(0));   MSC_REQUIRE_EQ(1.0 / 12.0, histo.frequency(0));
        MSC_REQUIRE_EQ(29.0, histo.center(1));   MSC_REQUIRE_EQ(0.0 / 12.0, histo.frequency(1));
        MSC_REQUIRE_EQ(30.0, histo.center(2));   MSC_REQUIRE_EQ(4.0 / 12.0, histo.frequency(2));
        MSC_REQUIRE_EQ(31.0, histo.center(3));   MSC_REQUIRE_EQ(7.0 / 12.0, histo.frequency(3));
    }

}  // namespace /*anonymous*/
