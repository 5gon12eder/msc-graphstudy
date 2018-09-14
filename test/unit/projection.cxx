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

#include "projection.hxx"

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(project_onto_plane_trivial)
    {
        constexpr auto normal = msc::point3d{0.0, 0.0, 1.0};
        constexpr auto p = msc::point3d{1.0, 2.0, 3.0};
        constexpr auto q = msc::project_onto_plane(p, normal);
        MSC_REQUIRE_CLOSE(1.0E-10, q.x(), p.x());
        MSC_REQUIRE_CLOSE(1.0E-10, q.y(), p.y());
        MSC_REQUIRE_CLOSE(1.0E-10, q.z(), 0.0);
        constexpr auto qq = msc::project_onto_plane(q, normal);
        MSC_REQUIRE_CLOSE(1.0E-10, qq.x(), p.x());
        MSC_REQUIRE_CLOSE(1.0E-10, qq.y(), p.y());
        MSC_REQUIRE_CLOSE(1.0E-10, qq.z(), 0.0);
    }

    MSC_AUTO_TEST_CASE(transform2d_trivial)
    {
        constexpr auto p = msc::point2d{1.4, 9.2};
        constexpr auto ex = msc::point2d{1.0, 0.0};
        constexpr auto ey = msc::point2d{0.0, 1.0};
        constexpr auto q = msc::transform2d(p, ex, ey);
        MSC_REQUIRE_CLOSE(1.0E-10, p.x(), q.x());
        MSC_REQUIRE_CLOSE(1.0E-10, p.y(), q.y());
    }

    MSC_AUTO_TEST_CASE(isometric_projection_origin_to_origin)
    {
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 0>{}));
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 1>{}));
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 2>{}));
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 3>{}));
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 4>{}));
        MSC_REQUIRE_EQ(msc::point2d{}, msc::isometric_projection(msc::point<double, 5>{}));
    }

    MSC_AUTO_TEST_CASE(axonometric_projection)
    {
        constexpr auto point = msc::point3d{1.0, 2.0, 3.0};
        constexpr auto actual = msc::axonometric_projection(msc::projections::isometric, point);
        const auto expected = msc::isometric_projection(point);  // not constexpr
        MSC_REQUIRE_CLOSE(1.0E-10, expected.x(), actual.x());
        MSC_REQUIRE_CLOSE(1.0E-10, expected.y(), actual.y());
    }

}  // namespace /*anonymous*/
