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

#include "json.hxx"

#include <limits>
#include <random>
#include <sstream>
#include <utility>

#include "unittest.hxx"

namespace /*anonymous*/
{

    template <typename T>
    std::string stringize(const T& obj)
    {
        auto oss = std::ostringstream{};
        oss << obj;
        return oss.str();
    }

    MSC_AUTO_TEST_CASE(json_null)
    {
        MSC_REQUIRE_EQ("null", stringize(msc::json_null{}));
    }

    MSC_AUTO_TEST_CASE(json_text_empty)
    {
        MSC_REQUIRE_EQ("\"\"", stringize(msc::json_text{}));
    }

    MSC_AUTO_TEST_CASE(json_text_non_empty)
    {
        MSC_REQUIRE_EQ("\"hello, world\"", stringize(msc::json_text{"hello, world"}));
    }

    MSC_AUTO_TEST_CASE(json_real_zero)
    {
        MSC_REQUIRE_MATCH(R"X(0\.0+E\+0+)X", stringize(msc::json_real{}));
    }

    MSC_AUTO_TEST_CASE(json_real_finite_1st)
    {
        MSC_REQUIRE_MATCH(R"X(4\.21250*E\+0*1)X", stringize(msc::json_real{42.125}));
    }

    MSC_AUTO_TEST_CASE(json_real_finite_2nd)
    {
        const auto minval = std::numeric_limits<double>::min();
        const auto maxval = std::numeric_limits<double>::max();
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution<double>{minval, maxval};
        for (auto i = 0; i < 100; ++i) {
            const auto value = rnddst(rndeng);
            const auto object = msc::json_real{value};
            const auto json = stringize(object);
            const auto back = std::stod(json);
            MSC_REQUIRE_CLOSE(1.0E-15, 0.0, (value == 0.0) ? back : std::abs(1.0 - back / value));
        }
    }

    MSC_AUTO_TEST_CASE(json_real_infinite)
    {
        const auto inf = std::numeric_limits<double>::infinity();
        MSC_REQUIRE_EQ( "Infinity", stringize(msc::json_real{+1.0 * inf}));
        MSC_REQUIRE_EQ("-Infinity", stringize(msc::json_real{-1.0 * inf}));
    }

    MSC_AUTO_TEST_CASE(json_real_nan)
    {
        const auto nan = std::numeric_limits<double>::quiet_NaN();
        MSC_REQUIRE_EQ("NaN", stringize(msc::json_real{-1.0 * nan}));
        MSC_REQUIRE_EQ("NaN", stringize(msc::json_real{+1.0 * nan}));
    }

    MSC_AUTO_TEST_CASE(json_size)
    {
        MSC_REQUIRE_EQ("0",  stringize(msc::json_size{}));
        MSC_REQUIRE_EQ("42", stringize(msc::json_size{42}));
    }

    MSC_AUTO_TEST_CASE(json_diff)
    {
        MSC_REQUIRE_EQ("0",  stringize(msc::json_diff{}));
        MSC_REQUIRE_EQ("13", stringize(msc::json_diff{13}));
        MSC_REQUIRE_EQ("-7", stringize(msc::json_diff{-7}));
    }

    MSC_AUTO_TEST_CASE(json_any)
    {
        MSC_REQUIRE_EQ("null", stringize(msc::json_any{}));
    }

    MSC_AUTO_TEST_CASE(json_any_null)
    {
        const msc::json_any anything = msc::json_null{nullptr};
        MSC_REQUIRE_EQ("null", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_text)
    {
        const msc::json_any anything = msc::json_text{"hello, world"};
        MSC_REQUIRE_EQ("\"hello, world\"", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_real)
    {
        const msc::json_any anything = msc::json_real{123.456789};
        MSC_REQUIRE_MATCH(R"X(1\.23456789\d*E\+0*2)X", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_size)
    {
        const msc::json_any anything = msc::json_size{42};
        MSC_REQUIRE_EQ("42", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_diff)
    {
        const msc::json_any anything = msc::json_diff{-42};
        MSC_REQUIRE_EQ("-42", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_array)
    {
        const msc::json_any anything = msc::json_array{};
        MSC_REQUIRE_MATCH(R"X(\[\s*\])X", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_any_object)
    {
        const msc::json_any anything = msc::json_object{};
        MSC_REQUIRE_MATCH(R"X(\{\s*\})X", stringize(anything));
    }

    MSC_AUTO_TEST_CASE(json_array_0th)
    {
        MSC_REQUIRE_MATCH(R"X(\[\s*\])X", stringize(msc::json_array{}));
    }

    MSC_AUTO_TEST_CASE(json_array_1st)
    {
        auto array = msc::json_array{};
        array.emplace_back(msc::json_size{1});
        MSC_REQUIRE_MATCH(R"X(\[\s*1\s*\])X", stringize(array));
    }

    MSC_AUTO_TEST_CASE(json_array_2nd)
    {
        auto array = msc::json_array{};
        array.emplace_back(msc::json_size{1});
        array.emplace_back(msc::json_size{2});
        MSC_REQUIRE_MATCH(R"X(\[\s*1,\s+2\s*\])X", stringize(array));
    }

    MSC_AUTO_TEST_CASE(json_array_3rd)
    {
        auto array = msc::json_array{};
        array.emplace_back(msc::json_null{});
        array.emplace_back(msc::json_array{});
        array.emplace_back(msc::json_object{});
        MSC_REQUIRE_MATCH(R"X(\[\s*null,\s+\[\s*\],\s+\{\s*\}\s*\])X", stringize(array));
    }

    MSC_AUTO_TEST_CASE(json_object_0th)
    {
        MSC_REQUIRE_MATCH(R"X(\{\s*\})X", stringize(msc::json_object{}));
    }

    MSC_AUTO_TEST_CASE(json_object_1st)
    {
        auto object = msc::json_object{};
        object["foo"] = msc::json_null{};
        MSC_REQUIRE_MATCH(R"X(\{\s*\"foo\"\s*:\s+null\s*\})X", stringize(object));
    }

    MSC_AUTO_TEST_CASE(json_object_2nd)
    {
        auto object = msc::json_object{};
        object["bar"] = msc::json_size{2};
        object["baz"] = msc::json_size{3};
        MSC_REQUIRE_MATCH(R"X(\{\s*\"bar\"\s*:\s+2,\s+\"baz\"\s*:\s+3\s*\})X", stringize(object));
    }

    MSC_AUTO_TEST_CASE(json_object_3rd)
    {
        auto object = msc::json_object{};
        object["alpha"] = msc::json_object{};
        object["beta"] = msc::json_array{};
        object["gamma"] = msc::json_null{};
        MSC_REQUIRE_MATCH(
            R"X(\{\s*\"alpha\"\s*:\s+\{\s*\},\s+\"beta\"\s*:\s+\[\s*\],\s+\"gamma\"\s*:\s+null\s*\})X",
            stringize(object)
        );
    }

    MSC_AUTO_TEST_CASE(json_object_transparent)
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        auto object = msc::json_object{};
        object["foo"]   = msc::json_real{1.0};
        object["foo"s]  = msc::json_real{2.0};
        object["foo"sv] = msc::json_real{3.0};
        MSC_REQUIRE_EQ(1, object.size());
    }

    MSC_AUTO_TEST_CASE(json_object_update_copy)
    {
        auto object = msc::json_object{};
        object["alpha"] = msc::json_size{13};
        object["beta"] = msc::json_text{"fancy stuff"};
        auto news = msc::json_object{};
        news["beta"] = msc::json_null{};
        news["gamma"] = msc::json_array{};
        MSC_REQUIRE_EQ(2, object.size());
        MSC_REQUIRE_EQ(2, news.size());
        object.update(std::as_const(news));
        MSC_REQUIRE_EQ(3, object.size());
        MSC_REQUIRE_EQ(2, news.size());
        MSC_REQUIRE_MATCH(
            R"regex(\{\s*"alpha"\s*:\s+13,\s+"beta"\s*:\s+null,\s+"gamma"\s*:\s+\[\s*\]\s*\})regex",
            stringize(object)
        );
    }

    MSC_AUTO_TEST_CASE(json_object_update_move)
    {
        auto object = msc::json_object{};
        object["alpha"] = msc::json_size{13};
        object["beta"] = msc::json_text{"fancy stuff"};
        auto news = msc::json_object{};
        news["beta"] = msc::json_null{};
        news["gamma"] = msc::json_array{};
        MSC_REQUIRE_EQ(2, object.size());
        MSC_REQUIRE_EQ(2, news.size());
        object.update(std::move(news));
        MSC_REQUIRE_EQ(3, object.size());
        MSC_REQUIRE_EQ(0, news.size());
        MSC_REQUIRE_MATCH(
            R"X(\{\s*"alpha"\s*:\s+13,\s+"beta"\s*:\s+null,\s+"gamma"\s*:\s+\[\s*\]\s*\})X",
            stringize(object)
        );
    }

}  // namespace /*anonymous*/
