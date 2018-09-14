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

#ifndef MSC_INCLUDED_FROM_CUBOID_HXX
#  error "Never `#include <cuboid.txx>` directly, `#include <cuboid.hxx>` instead"
#endif

#include <algorithm>
#include <cassert>
#include <cmath>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "projection.hxx"

namespace msc
{

    namespace detail::cuboid
    {

        template <typename IdxT, std::size_t... Is>
        constexpr std::array<IdxT, sizeof...(Is)>
        neighbours(const IdxT idx, std::index_sequence<Is...>) noexcept
        {
            return { (idx ^ (IdxT{1} << Is)) ... };
        }

        template <typename T, std::size_t... Is>
        constexpr typename cuboid<T, sizeof...(Is)>::point_type
        corner(const typename cuboid<T, sizeof...(Is)>::index_type  idx,
               const typename cuboid<T, sizeof...(Is)>::point_type& org,
               const typename cuboid<T, sizeof...(Is)>::point_type& ext,
               std::index_sequence<Is...>) noexcept
        {
            return { ((std::size_t{1} & (idx >> Is)) ? (get<Is>(org) + get<Is>(ext)) : get<Is>(org)) ... };
        }

    }  // namespace detail::cuboid

    template <typename T, std::size_t N>
    constexpr std::size_t cuboid<T, N>::dimensions() noexcept
    {
        return N;
    }

    template <typename T, std::size_t N>
    constexpr std::size_t cuboid<T, N>::corners() noexcept
    {
        return (std::size_t{1} << N);
    }

    template <typename T, std::size_t N>
    constexpr auto cuboid<T, N>::neighbours(const typename cuboid<T, N>::index_type idx)
        -> std::array<typename cuboid<T, N>::index_type, N>
    {
        assert(idx < cuboid::corners());
        return detail::cuboid::neighbours(idx, std::make_index_sequence<N>());
    }

    template <typename T, std::size_t N>
    constexpr typename cuboid<T, N>::point_type cuboid<T, N>::corner(const typename cuboid<T, N>::index_type idx) const
    {
        assert(idx < cuboid::corners());
        return detail::cuboid::corner<T>(idx, _origin, _extension, std::make_index_sequence<N>());
    }

    template <typename FwdIterT, typename CuboidT, typename PointT, typename T, std::size_t N>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    convert_and_project(const FwdIterT first, const FwdIterT last)
    {
        static_assert(std::is_same_v<CuboidT, typename std::iterator_traits<FwdIterT>::value_type>);
        static_assert(std::is_same_v<PointT, typename CuboidT::point_type>);
        static_assert(std::is_same_v<T, typename PointT::value_type>);
        static_assert(N == std::tuple_size<PointT>::value);
        constexpr auto zero = typename CuboidT::index_type{0};
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        for (auto iter = first; iter != last; ++iter) {
            std::array<ogdf::node, CuboidT::corners()> nodes;
            for (auto i = zero; i < CuboidT::corners(); ++i) {
                const auto corner = iter->corner(i);
                const auto v = nodes[i] = graph->newNode();
                const auto [x, y] = isometric_projection(corner);
                attrs->x(v) = x;
                attrs->y(v) = y;
                for (const auto j : iter->neighbours(i)) {
                    if (j < i) {
                        graph->newEdge(nodes[i], nodes[j]);
                    }
                }
            }
        }
        return {std::move(graph), std::move(attrs)};
    }

}  // namespace msc
