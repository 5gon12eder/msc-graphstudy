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

/**
 * @file projection.hxx
 *
 * @brief
 *     Mind-blowing projections and coordinate transformations in <var>N</var>-dimensional spaces.
 *
 */

#ifndef MSC_PROJECTION_HXX
#define MSC_PROJECTION_HXX

#include "point.hxx"

#include "enums/projections.hxx"

namespace msc
{

    /**
     * @brief
     *     Projects a point in <var>N</var>-dimensional space onto a 2-dimensional plane in that space.
     *
     * The projection plane is given by its (<var>N</var>-dimensional) normal vector and implicitly assumed to pass
     * through the origin.
     *
     * The behavior is undefined `normal` is not normalized to 1.
     *
     * @tparam T
     *     floating-point type of point coordinates
     *
     * @tparam N
     *     dimensionality of the space
     *
     * @param coords
     *     point to be projected
     *
     * @param normal
     *     surface normal of the projection plane
     *
     */
    template <typename T, std::size_t N, typename = std::enable_if_t<(N > 2)>>
    constexpr point<T, N> project_onto_plane(const point<T, N>& coords, const point<T, N>& normal);

    /**
     * @brief
     *     Returns the 2-dimensional coordinates of a point on a 2-dimensional plane in an <var>N</var>-dimensional
     *     space with respect to two freely chosen unit vectors.
     *
     * The projection plane is implicitly assumed to pass through the origin.  The behavior is undefined if it is
     * impossible for `e1, `e2` and `coords` to lie in such a plane or `e1` and `e2` are linearly dependant.
     *
     * @tparam T
     *     floating-point type of point coordinates
     *
     * @tparam N
     *     dimensionality of the space
     *
     * @param coords
     *     point on the plane
     *
     * @param e1
     *     first unit vector
     *
     * @param e2
     *     second unit vector
     *
     * @returns
     *     2-dimensional coordinates of `coords` with respect to the coordinate system defined by `e1` and `e2`
     *
     */
    template <typename T, std::size_t N, typename = std::enable_if_t<(N >= 2)>>
    constexpr point<T, 2> transform2d(const point<T, N>& coords, const point<T, N>& e1, const point<T, N>& e2);

    /**
     * @brief
     *     Returns the 2-dimensional coordinates of a <var>N</var>-dimensional point's isometric projection.
     *
     * @warning
     *     The coordinate computation will always be performed in double precision and the result casted to `T`
     *     afterwards.
     *
     * @tparam T
     *     floating-point type of point coordinates
     *
     * @tparam N
     *     dimensionality of the space
     *
     * @param coords
     *     point to be projected
     *
     * @returns
     *     2-dimensional coordinates on the drawing area
     *
     */
    template <typename T, std::size_t N>
    point<T, 2> isometric_projection(const point<T, N>& coords) noexcept;

    /**
     * @brief
     *     Returns the 2-dimensional coordinates of a 3-dimensional point's projection.
     *
     * The behavior is undefined if `type` is not a declared enumerator.
     *
     * If `type == projections::isometric`, this function is a more efficient version of `isometric_projection` for the
     * special case of a 3-dimensional space.
     *
     * @warning
     *     The coordinate computation will always be performed in double precision and the result casted to `T`
     *     afterwards.
     *
     * @tparam T
     *     floating-point type of point coordinates
     *
     * @param type
     *     type of projection
     *
     * @param coords
     *     point to be projected
     *
     * @returns
     *     2-dimensional coordinates on the drawing area
     *
     */
    template <typename T>
    constexpr point<T, 2> axonometric_projection(projections type, point<T, 3> coords);

}  // namespace msc

#define MSC_INCLUDED_FROM_PROJECTION_HXX
#include "projection.txx"
#undef MSC_INCLUDED_FROM_PROJECTION_HXX

#endif  // !defined(MSC_PROJECTION_HXX)
