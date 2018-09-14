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

#include "cuboid.hxx"

#include "file.hxx"
#include "io.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(trivial_things_3d)
    {
        using cuboid_type = msc::cuboid<float, 3>;
        using point_type = cuboid_type::point_type;
        using index_type = cuboid_type::index_type;
        constexpr auto org = point_type{1.0f, 2.0f, 3.0f};
        constexpr auto ext = point_type{4.0f, 5.0f, 6.0f};
        constexpr auto thing = cuboid_type{org, ext};
        static_assert(!cuboid_type{}.origin());
        static_assert(!cuboid_type{}.extension());
        static_assert(thing.origin() == org);
        static_assert(thing.extension() == ext);
        static_assert(cuboid_type::dimensions() == 3);
        static_assert(cuboid_type::corners() == 8);
        static_assert(cuboid_type::neighbours(0b011u).size() == 3);
        static_assert(cuboid_type::neighbours(0b011u)[0] == index_type{0b010u});
        static_assert(cuboid_type::neighbours(0b011u)[1] == index_type{0b001u});
        static_assert(cuboid_type::neighbours(0b011u)[2] == index_type{0b111u});
        static_assert(thing.corner(0b011u) == point_type{org[0] + ext[0], org[1] + ext[1], org[2]});
        static_assert(thing.corner(0b101u) == point_type{org[0] + ext[0], org[1], org[2] + ext[2]});
    }

    MSC_AUTO_TEST_CASE(convert_and_project_4d)
    {
        // TODO: This test hardly tests anything at all.
        constexpr auto dims = std::size_t{4};
        using cuboid_type = msc::cuboid<double, dims>;
        auto rndeng = std::default_random_engine{};
        auto orgdst = std::normal_distribution{0.0, 50.0};
        auto extdst = std::normal_distribution{0.0, 10.0};
        auto cubos = std::vector<cuboid_type>{};
        for (auto i = 0; i < 30; ++i) {
            const auto org = msc::make_random_point<double, dims>(rndeng, orgdst);
            const auto ext = msc::make_random_point<double, dims>(rndeng, extdst);
            cubos.emplace_back(org, ext);
        }
        const auto [graph, attrs] = msc::convert_and_project(std::cbegin(cubos), std::cend(cubos));
        MSC_REQUIRE_EQ(cubos.size() * cuboid_type::corners(), graph->numberOfNodes());
        MSC_REQUIRE_EQ(cubos.size() * dims * (1ul << (dims - 1)), graph->numberOfEdges());
        if (const auto dumpfilename = std::getenv("MSC_TEST_DUMP_CONVERT_AND_PROJECT_4D")) {
            const auto dest = msc::output_file{dumpfilename};
            msc::store_layout(*attrs, dest);
        }
    }

}  // namespace /*anonymous*/
