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

#include "useful.hxx"

#include <climits>
#include <string>
#include <string_view>

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(cyclic_next_one)
    {
        const auto text = std::string{"a"};
        MSC_REQUIRE_EQ('a', *msc::cyclic_next(text.begin(), text.begin(), text.end()));
    }

    MSC_AUTO_TEST_CASE(cyclic_next_two)
    {
        const auto text = std::string{"ab"};
        MSC_REQUIRE_EQ('b', *msc::cyclic_next(text.begin() + 0, text.begin(), text.end()));
        MSC_REQUIRE_EQ('a', *msc::cyclic_next(text.begin() + 1, text.begin(), text.end()));
    }

    MSC_AUTO_TEST_CASE(cyclic_next_three)
    {
        const auto text = std::string{"abc"};
        MSC_REQUIRE_EQ('b', *msc::cyclic_next(text.begin() + 0, text.begin(), text.end()));
        MSC_REQUIRE_EQ('c', *msc::cyclic_next(text.begin() + 1, text.begin(), text.end()));
        MSC_REQUIRE_EQ('a', *msc::cyclic_next(text.begin() + 2, text.begin(), text.end()));
    }

    MSC_AUTO_TEST_CASE(get_item)
    {
        using namespace std::string_literals;
        const auto nil = '\a';  // arbitrary character not occurring in any string
        MSC_REQUIRE_EQ(nil, msc::get_item(""s,     0).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item(""s,     1).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item(""s,    42).value_or(nil));
        MSC_REQUIRE_EQ('a', msc::get_item("a"s,    0).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("a"s,    1).value_or(nil));
        MSC_REQUIRE_EQ('a', msc::get_item("ab"s,   0).value_or(nil));
        MSC_REQUIRE_EQ('b', msc::get_item("ab"s,   1).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("ab"s,   2).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("ab"s,   3).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("ab"s,  10).value_or(nil));
        MSC_REQUIRE_EQ('a', msc::get_item("abc"s,  0).value_or(nil));
        MSC_REQUIRE_EQ('b', msc::get_item("abc"s,  1).value_or(nil));
        MSC_REQUIRE_EQ('c', msc::get_item("abc"s,  2).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("abc"s,  3).value_or(nil));
        MSC_REQUIRE_EQ(nil, msc::get_item("abc"s, 10).value_or(nil));
    }

    MSC_AUTO_TEST_CASE(get_same)
    {
        MSC_REQUIRE_EQ(14.92,   msc::get_same({14.92}));
        MSC_REQUIRE_EQ(1,       msc::get_same({1, 1}));
        MSC_REQUIRE_EQ('x',     msc::get_same({'x', 'x', 'x'}));
        MSC_REQUIRE_EQ(42,      msc::get_same({42, 42, 42, 42}));
        MSC_REQUIRE_EQ(nullptr, msc::get_same({nullptr, nullptr, nullptr, nullptr, nullptr}));
    }

    MSC_AUTO_TEST_CASE(optional_cast_value)
    {
        const std::optional<double> src = 13.7;
        const std::optional<int> dst = msc::optional_cast<int>(src);
        MSC_REQUIRE(dst.has_value());
        MSC_REQUIRE_EQ(13, dst.value());
    }

    MSC_AUTO_TEST_CASE(optional_cast_empty)
    {
        const std::optional<double> src = std::nullopt;
        const std::optional<int> dst = msc::optional_cast<int>(src);
        MSC_REQUIRE(!dst.has_value());
    }

    MSC_AUTO_TEST_CASE(share_pair_00)
    {
        using shared_pair = std::pair<std::shared_ptr<double>, std::shared_ptr<char>>;
        using unique_pair = std::pair<std::unique_ptr<double>, std::unique_ptr<char>>;
        unique_pair unique = {nullptr, nullptr};
        shared_pair shared = msc::share_pair(std::move(unique));
        MSC_REQUIRE_EQ(nullptr, unique.first.get());
        MSC_REQUIRE_EQ(nullptr, unique.second.get());
        MSC_REQUIRE_EQ(nullptr, shared.first.get());
        MSC_REQUIRE_EQ(nullptr, shared.second.get());
    }

    MSC_AUTO_TEST_CASE(share_pair_01)
    {
        using shared_pair = std::pair<std::shared_ptr<double>, std::shared_ptr<char>>;
        using unique_pair = std::pair<std::unique_ptr<double>, std::unique_ptr<char>>;
        unique_pair unique = {nullptr, std::make_unique<char>('A')};
        shared_pair shared = msc::share_pair(std::move(unique));
        MSC_REQUIRE_EQ(nullptr, unique.first.get());
        MSC_REQUIRE_EQ(nullptr, unique.second.get());
        MSC_REQUIRE_EQ(nullptr, shared.first.get());
        MSC_REQUIRE_NE(nullptr, shared.second.get());
        MSC_REQUIRE_EQ('A', *shared.second);
    }

    MSC_AUTO_TEST_CASE(share_pair_10)
    {
        using shared_pair = std::pair<std::shared_ptr<double>, std::shared_ptr<char>>;
        using unique_pair = std::pair<std::unique_ptr<double>, std::unique_ptr<char>>;
        unique_pair unique = {std::make_unique<double>(1.5), nullptr};
        shared_pair shared = msc::share_pair(std::move(unique));
        MSC_REQUIRE_EQ(nullptr, unique.first.get());
        MSC_REQUIRE_EQ(nullptr, unique.second.get());
        MSC_REQUIRE_NE(nullptr, shared.first.get());
        MSC_REQUIRE_EQ(nullptr, shared.second.get());
        MSC_REQUIRE_EQ(1.5, *shared.first);
    }

    MSC_AUTO_TEST_CASE(share_pair_11)
    {
        using shared_pair = std::pair<std::shared_ptr<double>, std::shared_ptr<char>>;
        using unique_pair = std::pair<std::unique_ptr<double>, std::unique_ptr<char>>;
        unique_pair unique = {std::make_unique<double>(1.5), std::make_unique<char>('A')};
        shared_pair shared = msc::share_pair(std::move(unique));
        MSC_REQUIRE_EQ(nullptr, unique.first.get());
        MSC_REQUIRE_EQ(nullptr, unique.second.get());
        MSC_REQUIRE_NE(nullptr, shared.first.get());
        MSC_REQUIRE_NE(nullptr, shared.second.get());
        MSC_REQUIRE_EQ(1.5, *shared.first);
        MSC_REQUIRE_EQ('A', *shared.second);
    }

    MSC_AUTO_TEST_CASE(normalize_constant_name)
    {
        MSC_REQUIRE_EQ("", msc::normalize_constant_name(""));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name("alpha"));
        MSC_REQUIRE_EQ("", msc::normalize_constant_name("  \t \t\t"));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name("   alpha"));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name("alpha   "));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name(" \t  alpha  \t\t"));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name("Alpha"));
        MSC_REQUIRE_EQ("alpha", msc::normalize_constant_name("ALPHA"));
        MSC_REQUIRE_EQ("a   a", msc::normalize_constant_name("\tA   a     "));
        MSC_REQUIRE_EQ("alpha-beta", msc::normalize_constant_name("Alpha_Beta"));
        MSC_REQUIRE_EQ("alpha-beta", msc::normalize_constant_name("Alpha-Beta"));
    }

    MSC_AUTO_TEST_CASE(reject_invalid_enumeration_enum)
    {
        using namespace std::string_view_literals;
        enum class moods { happy = 1, angry = 2, sad = 3 };
        try {
            msc::reject_invalid_enumeration(moods{}, "moods");
            MSC_FAIL("No exception was thrown");
        } catch (const std::invalid_argument& e) {
            MSC_REQUIRE_EQ("0 is not a valid constant for an enumerator of type 'moods'"sv, std::string_view{e.what()});
        }
    }

    MSC_AUTO_TEST_CASE(reject_invalid_enumeration_int)
    {
        using namespace std::string_view_literals;
        try {
            msc::reject_invalid_enumeration(42, "moods");
            MSC_FAIL("No exception was thrown");
        } catch (const std::invalid_argument& e) {
            MSC_REQUIRE_EQ(
                "42 is not a valid constant for an enumerator of type 'moods'"sv,
                std::string_view{e.what()}
            );
        }
    }

    MSC_AUTO_TEST_CASE(reject_invalid_enumeration_str)
    {
        using namespace std::string_view_literals;
        try {
            msc::reject_invalid_enumeration("dizzy", "moods");
            MSC_FAIL("No exception was thrown");
        } catch (const std::invalid_argument& e) {
            MSC_REQUIRE_EQ(
                "'dizzy' is not a valid name for an enumerator of type 'moods'"sv,
                std::string_view{e.what()}
            );
        }
    }

    MSC_AUTO_TEST_CASE(parse_decimal_number)
    {
        MSC_REQUIRE_EQ(0, msc::parse_decimal_number("0"));
        MSC_REQUIRE_EQ(1, msc::parse_decimal_number("1"));
        MSC_REQUIRE_EQ(42, msc::parse_decimal_number("42"));
        MSC_REQUIRE_EQ(INT_MAX, msc::parse_decimal_number(std::to_string(INT_MAX)));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number(""));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("-1"));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("Holger"));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("0x20"));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("2f"));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number(" 4"));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("4 "));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number(" "));
        MSC_REQUIRE_EQ(std::nullopt, msc::parse_decimal_number("99999999999999999999999999999999999999999999999"));
    }

    MSC_AUTO_TEST_CASE(square)
    {
        MSC_REQUIRE_EQ(0.0, msc::square(0.0));
        MSC_REQUIRE_EQ(1.0, msc::square(1.0));
        MSC_REQUIRE_EQ(4.0, msc::square(2.0));
        MSC_REQUIRE_EQ(9.0, msc::square(3.0));
    }

}  // namespace /*anonymous*/
