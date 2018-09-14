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

#ifndef MSC_INCLUDED_FROM_PROJECTION_HXX
#  error "Never `#include <projection.txx>` directly, `#include <projection.hxx>` instead"
#endif

#include <cassert>
#include <cmath>

#include "math_constants.hxx"

namespace msc
{

    template <typename T, std::size_t N, typename /*SFINAE*/>
    constexpr point<T, N> project_onto_plane(const point<T, N>& coords, const point<T, N>& normal)
    {
        //assert(std::abs(1.0 - abs(normal)) < 1.0E-10);
        return coords - dot(coords, normal) * normal;
    }

    template <typename T, std::size_t N, typename /*SFINAE*/>
    constexpr point<T, 2> transform2d(const point<T, N>& coords, const point<T, N>& ex, const point<T, N>& ey)
    {
        assert(ex);
        assert(ey);
        //assert(ex / abs(ex) != ey / abs(ey));
        return {dot(ex, coords), dot(ey, coords)};
    }

    template <typename T, std::size_t N>
    point<T, 2> isometric_projection(const point<T, N>& coords) noexcept
    {
        using float_type = T;
        using point_type = point<float_type, 2>;
        auto dest = point_type{};
        for (auto i = std::size_t{0}; i < N; ++i) {
            const auto angle = i * 2.0 * M_PI / N;
            const auto direction = point_type{float_type(std::sin(angle)), float_type(std::cos(angle))};
            dest += coords[i] * direction;
        }
        return dest;
    }

    namespace detail::projection
    {

        template <typename T>
        constexpr std::tuple<point<T, 2>, point<T, 2>, point<T, 2>>
        get_axonometric_units(const projections type) noexcept
        {
            using float_type = T;
            using point_type = point<float_type, 2>;
            using tuple_type = std::tuple<point_type, point_type, point_type>;
            constexpr auto ft0 = float_type(0);
            constexpr auto ft1 = float_type(1);
            constexpr auto pt00 = point_type{ft0, ft0};
            constexpr auto pt01 = point_type{ft0, ft1};
            constexpr auto pt10 = point_type{ft1, ft0};
            switch (type) {
            case projections::ortho_top:
                return tuple_type{pt10, pt01, pt00};
            case projections::ortho_front:
                return tuple_type{pt10, pt00, pt01};
            case projections::ortho_side:
                return tuple_type{pt00, pt10, pt01};
            case projections::isometric:
                return tuple_type{
                    point_type{float_type(+0x0.0000000000000p+0), float_type(+0x1.0000000000000p+0)},
                    point_type{float_type(+0x1.bb67ae8584cabp-1), float_type(-0x1.ffffffffffffcp-2)},
                    point_type{float_type(-0x1.bb67ae8584ca8p-1), float_type(-0x1.0000000000004p-1)},
                };
            }
            // For invalid projections, we simply pretend everything is drawn at the origin.
            // That's a nice and constexpr friendly form of UB.  :-)
            return tuple_type{point_type{}, point_type{}, point_type{}};
        }

    }  // namespace detail::projection

    template <typename T>
    constexpr point<T, 2> axonometric_projection(const projections type, const point<T, 3> coords)
    {
        const auto [ex, ey, ez] = detail::projection::get_axonometric_units<T>(type);
        return coords.x() * ex + coords.y() * ey + coords.z() * ez;
    }

}  // namespace msc
