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

#include "data_analysis.hxx"

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "json.hxx"

#include "testaux/tempfile.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(nothing_done_for_less_than_three)
    {
        auto data = std::vector<double>{};
        while (data.size() < 3) {
            for (const auto kern : msc::all_kernels()) {
                auto info = msc::json_object{};
                const auto analyzer = msc::data_analyzer{kern};
                const auto status = analyzer.analyze_oknodo(data, info, info);
                MSC_REQUIRE_EQ(false, status);
                MSC_REQUIRE_EQ(true, info.empty());
                MSC_REQUIRE_EXCEPTION(std::logic_error, analyzer.analyze(data, info, info));
                MSC_REQUIRE_EQ(true, info.empty());
            }
            data.push_back(1.0);
        }
    }

    MSC_AUTO_TEST_CASE(something_done_for_three)
    {
        const auto data = std::vector<double>{1.0, 2.0, 3.0};
        for (const auto kern : msc::all_kernels()) {
            const auto analyzer = msc::data_analyzer{kern};
            {
                auto info = msc::json_object{};
                const auto status = analyzer.analyze_oknodo(data, info, info);
                MSC_REQUIRE_EQ(true, status);
                MSC_REQUIRE_EQ(false, info.empty());
            }
            {
                auto info = msc::json_object{};
                analyzer.analyze(data, info, info);
                MSC_REQUIRE_EQ(false, info.empty());
            }
        }
    }

    std::vector<double> make_random_data(const std::size_t n = 100)
    {
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::normal_distribution<>{};
        auto data = std::vector<double>(n);
        std::generate(std::begin(data), std::end(data), [&rndeng, &rnddst](){ return rnddst(rndeng); });
        return data;
    }

    MSC_AUTO_TEST_CASE(histo_no_width_no_count)
    {
        const auto data = make_random_data();
        auto analyzer = msc::data_analyzer{msc::kernels::boxed};
        auto info = msc::json_object{};
        analyzer.analyze(data, info, info);
        MSC_REQUIRE_EQ(name(msc::binnings::scott_normal_reference), std::get<msc::json_text>(info["binning"]).value);
    }

    MSC_AUTO_TEST_CASE(histo_no_width_yes_count)
    {
        const auto bincount = 42;
        const auto data = make_random_data();
        auto analyzer = msc::data_analyzer{msc::kernels::boxed};
        auto info = msc::json_object{};
        analyzer.set_bins(bincount);
        analyzer.analyze(data, info, info);
        MSC_REQUIRE_EQ(name(msc::binnings::fixed_count), std::get<msc::json_text>(info["binning"]).value);
        MSC_REQUIRE_EQ(bincount, std::get<msc::json_size>(info["bincount"]).value);
    }

    MSC_AUTO_TEST_CASE(histo_yes_width_no_count)
    {
        const auto binwidth = 0.75;
        const auto data = make_random_data();
        auto analyzer = msc::data_analyzer{msc::kernels::boxed};
        auto info = msc::json_object{};
        analyzer.set_width(binwidth);
        analyzer.analyze(data, info, info);
        MSC_REQUIRE_EQ(name(msc::binnings::fixed_width), std::get<msc::json_text>(info["binning"]).value);
        MSC_REQUIRE_EQ(binwidth, std::get<msc::json_real>(info["binwidth"]).value);
    }

    MSC_AUTO_TEST_CASE(histo_yes_width_yes_count)
    {
        const auto data = make_random_data();
        auto analyzer = msc::data_analyzer{msc::kernels::boxed};
        auto info = msc::json_object{};
        analyzer.set_width(1.0);
        analyzer.set_bins(13);
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, analyzer.analyze(data, info, info));
    }

    msc::kernels random_kernel(std::mt19937& rndeng) noexcept
    {
        auto rnddst = std::uniform_int_distribution<std::size_t>{0, msc::all_kernels().size() - 1};
        const auto idx = rnddst(rndeng);
        return msc::all_kernels().at(idx);
    }

    MSC_AUTO_TEST_CASE(fuzzy)
    {
        auto engine = std::mt19937{};
        auto lucky = std::bernoulli_distribution{};
        auto mudist = std::uniform_real_distribution<double>{-1.0E10, +1.0E10};
        auto metadist = std::exponential_distribution<double>{};
        auto countdist = std::exponential_distribution<double>{1.0E-3};
        for (auto i = 0; i < 100; ++i) {
            const auto temp = msc::test::tempfile{"-fuzz.dat"};
            const auto mu = mudist(engine);
            const auto sigma = std::abs(mu * metadist(engine));
            auto valdist = std::normal_distribution{mu, sigma};
            auto data = std::vector<double>{};
            const auto count = static_cast<std::size_t>(countdist(engine));
            data.reserve(count);
            std::generate_n(std::back_inserter(data), count, [&engine, &valdist](){ return valdist(engine); });
            const auto kernel = random_kernel(engine);
            auto analyzer = msc::data_analyzer{kernel};
            auto minval = lucky(engine) ? std::nullopt : std::make_optional(valdist(engine));
            auto maxval = lucky(engine) ? std::nullopt : std::make_optional(valdist(engine));
            if (minval.value_or(DBL_MIN) > maxval.value_or(DBL_MAX)) {
                std::swap(minval, maxval);
            }
            analyzer.set_range(minval, maxval);
            if (lucky(engine)) {
                analyzer.set_width(std::abs(sigma * metadist(engine)));
            }
            if (lucky(engine)) {
                analyzer.set_output(msc::output_file::from_filename(temp.filename()));
            }
            auto info = msc::json_object{};
            const auto answer = analyzer.analyze_oknodo(data, info, info);
            MSC_REQUIRE_IMPLIES((count >= 3), answer);
            if (answer) {
                MSC_REQUIRE_EQ(count, std::get<msc::json_size>(info["size"]).value);
            } else {
                MSC_REQUIRE(info.empty());
            }
        }
    }

}  // namespace /*anonymous*/
