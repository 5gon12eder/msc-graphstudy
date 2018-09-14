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

#include "strings.hxx"

#include <string>
#include <string_view>

#include "unittest.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(concat_zero)
    {
        MSC_REQUIRE_EQ(""s, msc::concat());
    }

    MSC_AUTO_TEST_CASE(concat_one)
    {
        MSC_REQUIRE_EQ(""s, msc::concat(""));
        MSC_REQUIRE_EQ(""s, msc::concat(""s));
        MSC_REQUIRE_EQ(""s, msc::concat(""sv));
        MSC_REQUIRE_EQ("alpha"s, msc::concat("alpha"));
        MSC_REQUIRE_EQ("alpha"s, msc::concat("alpha"s));
        MSC_REQUIRE_EQ("alpha"s, msc::concat("alpha"sv));
    }

    MSC_AUTO_TEST_CASE(concat_two)
    {
        MSC_REQUIRE_EQ(""s, msc::concat("", ""));
        MSC_REQUIRE_EQ(""s, msc::concat("", ""s));
        MSC_REQUIRE_EQ(""s, msc::concat("", ""sv));
        MSC_REQUIRE_EQ(""s, msc::concat(""s, ""));
        MSC_REQUIRE_EQ(""s, msc::concat(""s, ""s));
        MSC_REQUIRE_EQ(""s, msc::concat(""s, ""sv));
        MSC_REQUIRE_EQ(""s, msc::concat(""sv, ""));
        MSC_REQUIRE_EQ(""s, msc::concat(""sv, ""s));
        MSC_REQUIRE_EQ(""s, msc::concat(""sv, ""sv));
        MSC_REQUIRE_EQ("abcdef"s, msc::concat("abc", "def"));
        MSC_REQUIRE_EQ(std::string(100, 'a'), msc::concat(std::string(42, 'a'), std::string(58, 'a')));
    }

    MSC_AUTO_TEST_CASE(concat_many)
    {
        MSC_REQUIRE_EQ(""s, msc::concat("", "", ""s, "", ""sv, ""sv, ""sv, "", ""s));
        MSC_REQUIRE_EQ("concatenation"s, msc::concat("", "con", "c"s, "a", ""sv, "tena"sv, "t"sv, "i", "on"s));
    }

    MSC_AUTO_TEST_CASE(startswith)
    {
        MSC_REQUIRE_EQ(true, msc::startswith("", ""));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", "happy"));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", "happ"));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", "hap"));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", "ha"));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", "h"));
        MSC_REQUIRE_EQ(true, msc::startswith("happy", ""));
        MSC_REQUIRE_EQ(false, msc::startswith("", "happy"));
        MSC_REQUIRE_EQ(false, msc::startswith("unhappy", "happy"));
        MSC_REQUIRE_EQ(false, msc::startswith("alpha", "beta"));
        MSC_REQUIRE_EQ(false, msc::startswith("abc", "cba"));
    }

    MSC_AUTO_TEST_CASE(endswith)
    {
        MSC_REQUIRE_EQ(true, msc::endswith("", ""));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", "happy"));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", "appy"));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", "ppy"));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", "py"));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", "y"));
        MSC_REQUIRE_EQ(true, msc::endswith("happy", ""));
        MSC_REQUIRE_EQ(false, msc::endswith("", "happy"));
        MSC_REQUIRE_EQ(false, msc::endswith("happy", "unhappy"));
        MSC_REQUIRE_EQ(false, msc::endswith("alpha", "beta"));
        MSC_REQUIRE_EQ(false, msc::endswith("abc", "cba"));
    }

}  // namespace /*anonymous*/
