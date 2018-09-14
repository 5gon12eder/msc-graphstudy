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
 * @file cuboid.hxx
 *
 * @brief
 *     <var>N</var>-dimensional cuboids and related functions.
 *
 */

#ifndef MSC_CUBOID_HXX
#define MSC_CUBOID_HXX

#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "ogdf_fwd.hxx"
#include "point.hxx"

namespace msc
{

    /**
     * @brief
     *     An axis-aligned <var>N</var>-dimensional cuboid.
     *
     * Corners of the cuboid are numbered using the following scheme: The corner with index zero is at the origin.  The
     * corner with index `i` has for the `j`-th dimension the `j`-th coordinate of the origin if the `j`-th bit in `i`
     * is zero and the `j`-th coordinate of the origin plus the `j`-th coordinate of the extension if the `j`-th bit in
     * `i` is one.  Two corners with indices `i1` and `i2` are adjacent if and only if `i1` and `i2` differ in exactly
     * one bit.
     *
     * @tparam T
     *     floating-point type used to represent a single coordinate
     *
     * @tparam N
     *     dimensionality of the cuboid
     *
     */
    template <typename T, std::size_t N>
    class cuboid final
    {

        static_assert(std::is_floating_point_v<T>, "Cuboids can only have real coordinates");
        static_assert((N >= 2) && (N <= 30), "Cuboids can only have 2 ... 30 dimensions");

    public:

        /** @brief Type of the cuboid's corners.  */
        using point_type = point<T, N>;

        /** @brief Unsigned integer type sufficient for enumerating all corners of the cuboid.  */
        using index_type = std::uint32_t;

        /**
         * @brief
         *     Constructs a degenerate cuboid with zero extension located at the origin.
         *
         */
        constexpr cuboid() noexcept = default;

        /**
         * @brief
         *     Constructs a cuboid with the given origin and extension.
         *
         * @param org
         *     origin (coordinate of the first corner)
         *
         * @param ext
         *     extension (vector from the first to its opposing corner)
         *
         */
        constexpr cuboid(const point_type& org, const point_type& ext) noexcept : _origin{org}, _extension{ext}
        {
        }

        /**
         * @brief
         *     Returns the dimensions of the cuboid.
         *
         * @returns
         *     `N`
         *
         */
        static constexpr std::size_t dimensions() noexcept;

        /**
         * @brief
         *     Returns the number of corners of the cuboid.
         *
         * @returns
         *     `2**N`
         *
         */
        static constexpr std::size_t corners() noexcept;

        /**
         * @brief
         *     Returns the indices of the corners which are adjacent to the corner with index `idx`.
         *
         * The behavior is undefined unless `0 <= idx < 2**N`.
         *
         * @param idx
         *     index of the corner
         *
         * @returns
         *     indices of the `N` adcacent corners
         *
         */
        static constexpr std::array<index_type, N> neighbours(index_type idx);

        /**
         * @brief
         *     Returns the coordinates of the corner with index `idx`.
         *
         * The behavior is undefined unless `0 <= idx < 2**N`.
         *
         * @param idx
         *     index of the corner
         *
         * @returns
         *     coordinates of the corner
         *
         */
        constexpr point_type corner(index_type idx) const;

        /**
         * @brief
         *     Returns a constant reference to the cuboid's origin.
         *
         * @returns
         *     coordinates of corner with index zero
         *
         */
        constexpr const point_type& origin() const noexcept { return _origin; }

        /**
         * @brief
         *     Returns a mutable reference to the cuboid's origin.
         *
         * @returns
         *     coordinates of corner with index zero
         *
         */
        constexpr point_type& origin() noexcept { return _origin; }

        /**
         * @brief
         *     Returns a constant reference to the cuboid's extension.
         *
         * @returns
         *     vector from the corner with index zero to its opposing corner
         *
         */
        constexpr const point_type& extension() const noexcept { return _extension; }

        /**
         * @brief
         *     Returns a mutable reference to the cuboid's extension.
         *
         * @returns
         *     vector from the corner with index zero to its opposing corner
         *
         */
        constexpr point_type& extension() noexcept { return _extension; }

    private:

        /** @brief Origin of the cuboid.  */
        point<T, N> _origin{};

        /** @brief Extension of the cuboid.  */
        point<T, N> _extension{};

    };  // class cuboid

    /**
     * @brief
     *     This function was considered useful at one point but is undocumented, untested and unused at the moment.
     *
     * @tparam FwdIterT
     *     forward-iterator type that dereferences to a `cuboid` instantiation
     *
     * @tparam CuboidT
     *     type of the cuboids to project
     *
     * @tparam PointT
     *     type of the cuboid's corners
     *
     * @tparam T
     *     type of the cuboid's corner's coordinates
     *
     * @tparam N
     *     dimensionality of the cuboid
     *
     * @param first
     *     iterator to the first cuboid to project
     *
     * @param last
     *     iterator past the last cuboid to project
     *
     * @returns
     *     graph and native layout derived from projecting the cuboids
     *
     */
    template
    <
        typename FwdIterT,
        typename CuboidT = typename std::iterator_traits<FwdIterT>::value_type,
        typename PointT = typename CuboidT::point_type,
        typename T = typename PointT::value_type,
        std::size_t N = std::tuple_size<PointT>::value
    >
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    convert_and_project(const FwdIterT first, const FwdIterT last);

}  // namespace msc

#define MSC_INCLUDED_FROM_CUBOID_HXX
#include "cuboid.txx"
#undef MSC_INCLUDED_FROM_CUBOID_HXX

#endif  // !defined(MSC_CUBOID_HXX)
