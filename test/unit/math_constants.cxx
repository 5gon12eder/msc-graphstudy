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

#include "math_constants.hxx"

#include "unittest.hxx"

namespace /*anonymous*/
{

    constexpr double cx_abs(const double x) noexcept
    {
        return (x < 0.0) ? -x : x;
    }

    MSC_AUTO_TEST_CASE(constexpr_checks)
    {
        static_assert(cx_abs(M_PI    - 3.141592653589793) <= 1.0E-15);
        static_assert(cx_abs(M_E     - 2.718281828459045) <= 1.0E-15);
        static_assert(cx_abs(M_SQRT2 - 1.414213562373095) <= 1.0E-15);
    }

    MSC_AUTO_TEST_CASE(dynamic_checks)
    {
        MSC_REQUIRE_CLOSE(1.0E-15, M_PI, 4.0 * std::atan(1.0));
        MSC_REQUIRE_CLOSE(1.0E-15, M_E, std::exp(1.0));
        MSC_REQUIRE_CLOSE(1.0E-15, M_SQRT2, std::sqrt(2.0));
    }

}  // namespace /*anonymous*/
