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

#ifndef MSC_INCLUDED_FROM_PRINCOMP_HXX
#  error "Never `#include <princomp.txx>` directly, `#include <princomp.hxx>` instead"
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>

namespace msc
{

    namespace detail::princomp
    {

        template <typename T, std::size_t N, typename EngineT>
        point<T, N> make_random_unit_vector(EngineT& engine)
        {
            auto unidist = std::uniform_real_distribution{};
            while (true) {
                if (const auto r = make_random_point<T, N>(engine, unidist)) {
                    return r / abs(r);
                }
            }
        }

        template
        <
            typename EngineT,
            typename FwdIterT,
            typename PointT = typename std::iterator_traits<FwdIterT>::value_type,
            typename T = typename PointT::value_type,
            std::size_t N = std::tuple_size<PointT>::value
        >
        PointT find_primary_component(const FwdIterT first, const FwdIterT last, EngineT& engine)
        {
            // https://en.wikipedia.org/wiki/Principal_component_analysis#Iterative_computation
            // TODO: Make the iteration count adaptive!
            const auto limit = 100;
            const auto delta = std::sqrt(std::numeric_limits<T>::epsilon());
            const auto normalized = [](const PointT& x){ return x ? x / abs(x) : PointT{}; };
            auto r = detail::princomp::make_random_unit_vector<T, N>(engine);
            for (auto i = 0; i < limit; ++i) {
                const auto binop = [r](const PointT& accu, const PointT& x){ return accu + x * dot(x, r); };
                const auto s = normalized(std::accumulate(first, last, PointT{}, binop));
                if (distance(s, r) / abs(r) <= delta) {
                    return s;
                }
                r = s;
            }
            return r;
        }

    }  // namespace detail::princomp

    template <typename EngineT, typename FwdIterT, typename PointT, typename T, std::size_t N, std::size_t M>
    std::array<PointT, M> find_primary_axes(const FwdIterT first,
                                            const FwdIterT last,
                                            EngineT& engine,
                                            std::integral_constant<std::size_t, M>)
    {
        static_assert(M <= N, "The number of primary axes cannot be larger than the dimension of the feature space");
        auto components = std::array<PointT, M>{};
        auto mean = std::accumulate(first, last, PointT{}) / static_cast<T>(std::distance(first, last));
        std::transform(first, last, first, [mean](const PointT& x){ return x - mean; });
        for (auto& pc : components) {
            pc = detail::princomp::find_primary_component(first, last, engine);
            const auto gsproj = [pc](const PointT& x){ return x - pc * dot(pc, x); };  // Gram-Schmidt
            std::transform(first, last, first, gsproj);
        }
        return components;
    }

    template <typename FwdIterT, typename PointT, typename T, std::size_t N>
    void transform_basis(const FwdIterT first, const FwdIterT last, const std::array<PointT, N>& matrix)
    {
        for (auto it = first; it != last; ++it) {
            const auto x = *it;
            for (std::size_t i = 0; i < N; ++i) {
                (*it)[i] = dot(matrix[i], x);
            }
        }
    }

}  // namespace msc
