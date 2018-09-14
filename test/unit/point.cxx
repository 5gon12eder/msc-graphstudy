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

#include <cfloat>
#include <iomanip>
#include <random>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "point.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(operations)
    {
        // default c'tor 2d
        {
            constexpr auto p = msc::point2d{};
            static_assert(p.x() == 0.0);
            static_assert(p.y() == 0.0);
        }
        // default c'tor 3d
        {
            constexpr auto p = msc::point3d{};
            static_assert(p.x() == 0.0);
            static_assert(p.y() == 0.0);
            static_assert(p.z() == 0.0);
        }
        // c'tor 2d
        {
            constexpr auto p = msc::point2d{1.2, 3.4};
            static_assert(p.x() == 1.2);
            static_assert(p.y() == 3.4);
        }
        // c'tor 3d
        {
            constexpr auto p = msc::point3d{1.2, 3.4, 5.6};
            static_assert(p.x() == 1.2);
            static_assert(p.y() == 3.4);
            static_assert(p.z() == 5.6);
        }
        // equality 2d
        static_assert(msc::point2d{} == msc::point2d{});
        static_assert(!(msc::point2d{} != msc::point2d{}));
        static_assert(msc::point2d{1.0, 2.0} == msc::point2d{1.0, 2.0});
        static_assert(msc::point2d{1.0, 2.0} != msc::point2d{1.1, 2.0});
        static_assert(msc::point2d{1.0, 2.0} != msc::point2d{1.0, 2.1});
        // equality 3d
        static_assert(msc::point3d{} == msc::point3d{});
        static_assert(!(msc::point3d{} != msc::point3d{}));
        static_assert(msc::point3d{1.0, 2.0, 3.0} == msc::point3d{1.0, 2.0, 3.0});
        static_assert(msc::point3d{1.0, 2.0, 3.0} != msc::point3d{1.1, 2.0, 3.0});
        static_assert(msc::point3d{1.0, 2.0, 3.0} != msc::point3d{1.0, 2.1, 3.0});
        static_assert(msc::point3d{1.0, 2.0, 3.0} != msc::point3d{1.0, 2.0, 3.1});
        // addition 2d
        static_assert(msc::point2d{} + msc::point2d{} == msc::point2d{});
        static_assert(msc::point2d{1.0, 2.0} + msc::point2d{3.0, 4.0} == msc::point2d{4.0, 6.0});
        // addition 3d
        static_assert(msc::point3d{} + msc::point3d{} == msc::point3d{});
        static_assert(msc::point3d{1.0, 2.0, 3.0} + msc::point3d{4.0, 5.0, 6.0} == msc::point3d{5.0, 7.0, 9.0});
        // subtraction 2d
        static_assert(msc::point2d{} - msc::point2d{} == msc::point2d{});
        static_assert(msc::point2d{1.5, 2.0} - msc::point2d{1.0, 2.0} == msc::point2d{0.5, 0.0});
        // subtraction 3d
        static_assert(msc::point3d{} - msc::point3d{} == msc::point3d{});
        static_assert(msc::point3d{1.0, 2.0, 3.0} - msc::point3d{0.125, 0.25, 0.5} == msc::point3d{0.875, 1.75, 2.5});
        // scalar multiplication 2d
        static_assert(0.5 * msc::point2d{2.0, 3.0} == msc::point2d{1.0, 1.5});
        // scalar multiplication 3d
        static_assert(0.5 * msc::point3d{2.0, 3.0, 4.0} == msc::point3d{1.0, 1.5, 2.0});
        // scalar division 2d
        static_assert(msc::point2d{2.0, 3.0} / 2.0 == msc::point2d{1.0, 1.5});
        // scalar division 3d
        static_assert(msc::point3d{2.0, 3.0, 4.0} / 2.0 == msc::point3d{1.0, 1.5, 2.0});
        // boolean conversion 2d
        static_assert(msc::point2d{1.0, 2.0});
        static_assert(msc::point2d{0.0, DBL_EPSILON});
        static_assert(msc::point2d{DBL_EPSILON, 0.0});
        static_assert(msc::point2d{-DBL_EPSILON, -DBL_EPSILON});
        static_assert(!msc::point2d{0.0, 0.0});
        static_assert(!msc::point2d{1.0, NAN});
        static_assert(!msc::point2d{NAN, 1.0});
        static_assert(!msc::point2d{NAN, NAN});
        static_assert(!msc::point2d{});
        // boolean conversion 3d
        static_assert(msc::point3d{1.0, 2.0, 3.0});
        static_assert(msc::point3d{0.0, 0.0, DBL_EPSILON});
        static_assert(msc::point3d{0.0, DBL_EPSILON, 0.0});
        static_assert(msc::point3d{DBL_EPSILON, 0.0, 0.0});
        static_assert(!msc::point3d{0.0, 0.0, 0.0});
        static_assert(!msc::point3d{1.0, 1.0, NAN});
        static_assert(!msc::point3d{1.0, NAN, 1.0});
        static_assert(!msc::point3d{1.0, NAN, NAN});
        static_assert(!msc::point3d{NAN, 1.0, NAN});
        static_assert(!msc::point3d{NAN, NAN, 1.0});
        static_assert(!msc::point3d{NAN, NAN, NAN});
        static_assert(!msc::point3d{});
        // dot-product 2d
        static_assert(dot(msc::point2d{1.0, 2.0}, msc::point2d{3.0, 4.0}) == 11.0);
        // dot-product 3d
        static_assert(dot(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{4.0, 5.0, 6.0}) == 32.0);
        // norm 2d
        MSC_REQUIRE(abs(msc::point2d{}) == 0.0);
        MSC_REQUIRE_CLOSE(1.0E-10, abs(msc::point2d{3.0, 4.0}), 5.0);
        // norm 3d
        MSC_REQUIRE(abs(msc::point3d{}) == 0.0);
        MSC_REQUIRE_CLOSE(1.0E-10, abs(msc::point3d{2.0, 3.0, 4.0}), std::sqrt(29.0));
        // distance 2d
        MSC_REQUIRE(distance(msc::point2d{}, msc::point2d{}) == 0.0);
        MSC_REQUIRE(distance(msc::point2d{1.0, 2.0}, msc::point2d{1.0, 2.0}) == 0.0);
        MSC_REQUIRE_CLOSE(1.0E-10, distance(msc::point2d{1.0, 2.0}, msc::point2d{2.0, 1.0}), std::sqrt(2.0));
        // distance 3d
        MSC_REQUIRE(distance(msc::point3d{}, msc::point3d{}) == 0.0);
        MSC_REQUIRE(distance(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 2.0, 3.0}) == 0.0);
        MSC_REQUIRE_CLOSE(1.0E-10, distance(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{3.0, 2.0, 1.0}), std::sqrt(8.0));
        // cross-product
        static_assert(cross(msc::point3d{}, msc::point3d{}) == msc::point3d{});
        static_assert(cross(msc::point3d{1.0, 0.0, 0.0}, msc::point3d{0.0, 1.0, 0.0}) == msc::point3d{0.0, 0.0, 1.0});
        static_assert(cross(msc::point3d{0.0, 1.0, 0.0}, msc::point3d{1.0, 0.0, 0.0}) == msc::point3d{0.0, 0.0, -1.0});
    }

    MSC_AUTO_TEST_CASE(ostr_2d)
    {
        const auto expected = msc::point2d{1.0, 0.5};
        auto oss = std::ostringstream{};
        MSC_REQUIRE(oss << std::setprecision(2) << std::fixed << expected);
        const auto text = oss.str();
        MSC_REQUIRE(text == "(1.00, 0.50)");
        auto iss = std::istringstream{text};
        auto actual = msc::point2d{};
        MSC_REQUIRE(iss >> actual);
        MSC_REQUIRE(iss.eof());
        MSC_REQUIRE_EQ(expected, actual);
    }

    MSC_AUTO_TEST_CASE(ostr_3d)
    {
        const auto expected = msc::point3d{1.0, 0.5, 0.0};
        auto oss = std::ostringstream{};
        MSC_REQUIRE(oss << std::setprecision(5) << std::fixed << expected);
        const auto text = oss.str();
        MSC_REQUIRE(oss.str() == "(1.00000, 0.50000, 0.00000)");
        auto iss = std::istringstream{text};
        auto actual = msc::point3d{};
        MSC_REQUIRE(iss >> actual);
        MSC_REQUIRE(iss.eof());
        MSC_REQUIRE_EQ(expected, actual);
    }

    template <typename T, std::size_t N, typename RndEngT, typename RndDstT>
    void test_lexical_cast(RndEngT& rndeng, RndDstT& rnddst)
    {
        using point_type = msc::point<T, N>;
        for (auto i = 0; i < 100; ++i) {
            auto textual = std::string{};
            try {
                const auto expected = msc::make_random_point<T, N>(rndeng, rnddst);
                textual = boost::lexical_cast<std::string>(expected);
                const auto actual = boost::lexical_cast<point_type>(textual);
                if (abs(expected) > 1.0E-10) {
                    MSC_REQUIRE_LE(distance(expected, actual) / abs(expected), 1.0E-4);
                } else {
                    MSC_REQUIRE_LE(distance(expected, actual), 1.0E-11);
                }
            } catch (const boost::bad_lexical_cast&) {
                MSC_FAIL("Bad lexical cast: " + textual);
            }
        }
    }

    MSC_AUTO_TEST_CASE(lexical_cast)
    {
        auto rndeng = std::default_random_engine{};
        auto rnddst = std::uniform_real_distribution<double>{-1.0E+10, +1.0E+10};
        test_lexical_cast<double, 0>(rndeng, rnddst);
        test_lexical_cast<double, 1>(rndeng, rnddst);
        test_lexical_cast<double, 2>(rndeng, rnddst);
        test_lexical_cast<double, 3>(rndeng, rnddst);
        test_lexical_cast<double, 5>(rndeng, rnddst);
        test_lexical_cast<double, 9>(rndeng, rnddst);
    }

    MSC_AUTO_TEST_CASE(order)
    {
        constexpr auto comp = msc::point_order<double>{};
        static_assert(!comp(msc::point2d{}, msc::point2d{}));
        static_assert(!comp(msc::point3d{}, msc::point3d{}));
        static_assert(!comp(msc::point2d{1.0, 2.0}, msc::point2d{0.0, 0.0}));
        static_assert(!comp(msc::point2d{1.0, 2.0}, msc::point2d{0.0, 2.0}));
        static_assert(!comp(msc::point2d{1.0, 2.0}, msc::point2d{0.0, 3.0}));
        static_assert(!comp(msc::point2d{1.0, 2.0}, msc::point2d{1.0, 0.0}));
        static_assert(!comp(msc::point2d{1.0, 2.0}, msc::point2d{1.0, 2.0}));
        static_assert( comp(msc::point2d{1.0, 2.0}, msc::point2d{1.0, 3.0}));
        static_assert( comp(msc::point2d{1.0, 2.0}, msc::point2d{2.0, 0.0}));
        static_assert( comp(msc::point2d{1.0, 2.0}, msc::point2d{2.0, 2.0}));
        static_assert( comp(msc::point2d{1.0, 2.0}, msc::point2d{2.0, 3.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 1.0, 2.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 1.0, 3.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 1.0, 4.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 2.0, 2.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 2.0, 3.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 2.0, 4.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 3.0, 2.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 3.0, 3.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{0.0, 3.0, 4.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 1.0, 2.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 1.0, 3.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 1.0, 4.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 2.0, 2.0}));
        static_assert(!comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 2.0, 3.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 2.0, 4.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 3.0, 2.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 3.0, 3.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{1.0, 3.0, 4.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 1.0, 2.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 1.0, 3.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 1.0, 4.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 2.0, 2.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 2.0, 3.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 2.0, 4.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 3.0, 2.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 3.0, 3.0}));
        static_assert( comp(msc::point3d{1.0, 2.0, 3.0}, msc::point3d{2.0, 3.0, 4.0}));
    }

    MSC_AUTO_TEST_CASE(maker)
    {
        static_assert(msc::make_point<float, 0>(3.14f) == msc::point<float, 0>{});
        static_assert(msc::make_point<float, 1>(3.14f) == msc::point<float, 1>{3.14f});
        static_assert(msc::make_point<float, 2>(3.14f) == msc::point<float, 2>{3.14f, 3.14f});
        static_assert(msc::make_point<float, 3>(3.14f) == msc::point<float, 3>{3.14f, 3.14f, 3.14f});
    }

    MSC_AUTO_TEST_CASE(unit_maker)
    {
        static_assert(msc::make_unit_point<float, 1>(0) == msc::point<float, 1>{1.0f});
        static_assert(msc::make_unit_point<float, 2>(0) == msc::point<float, 2>{1.0f, 0.0f});
        static_assert(msc::make_unit_point<float, 2>(1) == msc::point<float, 2>{0.0f, 1.0f});
        static_assert(msc::make_unit_point<float, 5>(3) == msc::point<float, 5>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f});
    }

}
