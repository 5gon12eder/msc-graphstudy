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
 * @file princomp.hxx
 *
 * @brief
 *     Principal component analysis.
 *
 */

#ifndef MSC_PRINCOMP_HXX
#define MSC_PRINCOMP_HXX

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

#include "point.hxx"

namespace msc
{

    /**
     * @brief
     *     Performs a principal component analysis of an <var>N</var>-dimensional point cloud.
     *
     * The algorithm uses Gram-Schmidt with power iteration and is therefore non-deterministic which is why this
     * function accept a random engine as parameter.
     *
     * The coordinates in the range `[first, last)` are modified during the process.  On exit, the range will contain
     * the coordinates with the first `M` principal components subtracted.
     *
     * @tparam EngineT
     *     type of the random engine
     *
     * @tparam FwdIterT
     *     forward-iterator type of the iterator pair specifying the range of points
     *
     * @tparam PointT
     *     type of the <var>N</var>-dimensional point
     *
     * @tparam T
     *     floating-point type of the <var>N</var>-dimensional point's coordinates
     *
     * @tparam N
     *     dimensionality of the <var>N</var>-dimensional points
     *
     * @tparam M
     *     number of principal components to compute (`M <= N`)
     *
     * @param first
     *     iterator to the first point to consider
     *
     * @param last
     *     iterator past the last point to consider
     *
     * @param engine
     *     (pseudo) random engine to use
     *
     * @param dimensions
     *     constant to select the number of components that shall be determined
     *
     * @returns
     *     array of the first `M` principal component vectors (axes)
     *
     */
    template
    <
        typename EngineT,
        typename FwdIterT,
        typename PointT = typename std::iterator_traits<FwdIterT>::value_type,
        typename T = typename PointT::value_type,
        std::size_t N = std::tuple_size<PointT>::value,
        std::size_t M = N
    >
    std::array<PointT, M> find_primary_axes(const FwdIterT first, const FwdIterT last, EngineT& engine,
                                            std::integral_constant<std::size_t, M> dimensions = {});

    /**
     * @brief
     *     Performs a principal component analysis of an <var>N</var>-dimensional point cloud.
     *
     * This is a convenience overload for the function that takes a pair of iterators, accepting a container which
     * supports `std::begin()` and `std::end()` instead.
     *
     * The coordinates in the container are modified during the process.  On exit, the container will contain the
     * coordinates with the first `M` principal components subtracted.
     *
     * @tparam EngineT
     *     type of the random engine
     *
     * @tparam ContainerT
     *     type of the container holding the points
     *
     * @tparam PointT
     *     type of the <var>N</var>-dimensional point
     *
     * @tparam T
     *     floating-point type of the <var>N</var>-dimensional point's coordinates
     *
     * @tparam N
     *     dimensionality of the <var>N</var>-dimensional points
     *
     * @tparam M
     *     number of principal components to compute (`M <= N`)
     *
     * @param coords
     *     container holding points
     *
     * @param engine
     *     (pseudo) random engine to use
     *
     * @param dimensions
     *     constant to select the number of components that shall be determined
     *
     * @returns
     *     array of the first `M` principal component vectors (axes)
     *
     */
    template
    <
        typename EngineT,
        typename ContainerT,
        typename PointT = typename ContainerT::value_type,
        typename T = typename PointT::value_type,
        std::size_t N = std::tuple_size<PointT>::value,
        std::size_t M = N
    >
    std::array<PointT, M> find_primary_axes(ContainerT& coords, EngineT& engine,
                                            std::integral_constant<std::size_t, M> dimensions = {})
    {
        return find_primary_axes(std::begin(coords), std::end(coords), engine, dimensions);
    }

    /**
     * @brief
     *     Performs a principal component analysis of an <var>N</var>-dimensional point cloud.
     *
     * This is a convenience overload for the function that takes a pair of iterators, accepting a container which
     * supports `std::begin()` and `std::end()` instead.  Furthermore, the container is passed by-value so the original
     * container will not be modified.  Instead, a copy will be made.
     *
     * @tparam EngineT
     *     type of the random engine
     *
     * @tparam ContainerT
     *     type of the container holding the points
     *
     * @tparam PointT
     *     type of the <var>N</var>-dimensional point
     *
     * @tparam T
     *     floating-point type of the <var>N</var>-dimensional point's coordinates
     *
     * @tparam N
     *     dimensionality of the <var>N</var>-dimensional points
     *
     * @tparam M
     *     number of principal components to compute (`M <= N`)
     *
     * @param coords
     *     container holding points
     *
     * @param engine
     *     (pseudo) random engine to use
     *
     * @param dimensions
     *     constant to select the number of components that shall be determined
     *
     * @returns
     *     array of the first `M` principal component vectors (axes)
     *
     */
    template
    <
        typename EngineT,
        typename ContainerT,
        typename PointT = typename ContainerT::value_type,
        typename T = typename PointT::value_type,
        std::size_t N = std::tuple_size<PointT>::value,
        std::size_t M = N
    >
    std::array<PointT, M> find_primary_axes_nondestructive(ContainerT coords, EngineT& engine,
                                                           std::integral_constant<std::size_t, M> dimensions = {})
    {
        return find_primary_axes(std::begin(coords), std::end(coords), engine, dimensions);
    }

}  // namespace msc

#define MSC_INCLUDED_FROM_PRINCOMP_HXX
#include "princomp.txx"
#undef MSC_INCLUDED_FROM_PRINCOMP_HXX

#endif  // !defined(MSC_PRINCOMP_HXX)
