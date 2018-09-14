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

#include "random.hxx"

#include <iomanip>
#include <ios>
#include <random>
#include <sstream>

#include "unittest.hxx"
#include "testaux/envguard.hxx"

#define CAN_RESTORE_ENVIRONMENT  (HAVE_POSIX_GETENV && HAVE_POSIX_GETENV && HAVE_POSIX_GETENV)

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(seed_deterministic)
    {
        MSC_SKIP_UNLESS(CAN_RESTORE_ENVIRONMENT);
        const std::string values[] = {"", "1492", "The telegraph ain't doing me no good!"};
        for (const auto& value : values) {
            auto guard = msc::test::envguard{"MSC_RANDOM_SEED"};
            guard.set(value);
            auto engine1st = std::mt19937{};
            auto engine2nd = std::mt19937{};
            const auto seed1st = msc::seed_random_engine(engine1st);
            const auto seed2nd = msc::seed_random_engine(engine2nd);
            MSC_REQUIRE_EQ(seed1st, seed2nd);
            MSC_REQUIRE_EQ(engine1st, engine2nd);
        }
    }

    MSC_AUTO_TEST_CASE(seed_nondeterministic)
    {
        MSC_SKIP_UNLESS(CAN_RESTORE_ENVIRONMENT);
        auto guard = msc::test::envguard{"MSC_RANDOM_SEED"};
        guard.unset();
        auto engine1st = std::mt19937{};
        auto engine2nd = std::mt19937{};
        const auto seed1st = msc::seed_random_engine(engine1st);
        const auto seed2nd = msc::seed_random_engine(engine2nd);
        MSC_REQUIRE_NE(seed1st, seed2nd);
        MSC_REQUIRE_NE(engine1st, engine2nd);
    }

    MSC_AUTO_TEST_CASE(random_hex_string_1st)
    {
        using namespace std::string_literals;
        auto engine = std::mt19937{};
        const auto token = msc::random_hex_string(engine);
        MSC_REQUIRE_EQ("5cf6ee792cdf05e1ba2b6325c41a5f10"s, token);
    }

    MSC_AUTO_TEST_CASE(random_hex_string_2nd)
    {
        using namespace std::string_literals;
        auto engine = std::mt19937{};
        const auto token = msc::random_hex_string(engine, 5);
        MSC_REQUIRE_EQ("5cf6ee792c"s, token);
    }

    MSC_AUTO_TEST_CASE(random_hex_string_3rd)
    {
        using namespace std::string_literals;
        auto engine = std::mt19937{};
        const auto token = msc::random_hex_string(engine, 25);
        MSC_REQUIRE_EQ("5cf6ee792cdf05e1ba2b6325c41a5f10e7e459faa111b337aa"s, token);
    }

    MSC_AUTO_TEST_CASE(random_hex_string_4th)
    {
        using namespace std::string_literals;
        auto engine = std::mt19937{};
        const auto token = msc::random_hex_string(engine, 0);
        MSC_REQUIRE_EQ(""s, token);
    }

    MSC_AUTO_TEST_CASE(random_hex_string_reference)
    {
        using namespace std::string_literals;
        const auto n = std::size_t{42};
        const auto expected = [n](){
            auto engine = std::mt19937{};
            auto oss = std::ostringstream{};
            for (std::size_t i = 0; i < n; ++i) {
                const auto byte = (engine() & 0xffU);
                oss << std::hex << std::setw(2) << std::setfill('0') << byte;
            }
            return oss.str();
        }();
        const auto actual = [n](){
            auto engine = std::mt19937{};
            return msc::random_hex_string(engine, n);
        }();
        MSC_REQUIRE_EQ(expected, actual);
    }

}  // namespace /*anonymous*/
