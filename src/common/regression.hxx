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
 * @file regression.hxx
 *
 * @brief
 *     Very simple linear regression.
 *
 */

#ifndef MSC_REGRESSION_HXX
#define MSC_REGRESSION_HXX

#include <array>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace msc
{

    /**
     * @brief
     *     Computes a simple linear regression through a data set given by a list of of unordered
     *     (<var>x</var>, <var>y</var>)-pairs.
     *
     * All data points are given equal (unit) weight.
     *
     * If the data set ist empty, (NaN, NaN) is returned.
     *
     * @tparam FwdIterT
     *     forward iterator type
     *
     * @tparam PairT
     *     2-tuple-like type holding two floating-point values
     *
     * @tparam TX
     *     type of the <var>x</var> values
     *
     * @tparam TY
     *     type of the <var>y</var> values
     *
     * @tparam T
     *     type of the computed intercept and incline
     *
     * @param first
     *     iterator to first data point
     *
     * @param last
     *     iterator after last data point
     *
     * @returns
     *     pair of intercept and incline
     *
     */
    template
    <
        typename FwdIterT,
        typename PairT = typename std::iterator_traits<FwdIterT>::value_type,
        typename TX = typename std::tuple_element<0, PairT>::type,
        typename TY = typename std::tuple_element<1, PairT>::type,
        typename T = std::common_type_t<TX, TY>,
        typename = std::enable_if_t<std::tuple_size<PairT>::value == 2 and std::is_floating_point_v<T>>
    >
    std::array<T, 2> linear_regression(FwdIterT first, FwdIterT last);

    /**
     * @brief
     *     Computes a simple linear regression through a data set given by a list of of unordered
     *     (<var>x</var>, <var>y</var>)-pairs.
     *
     * This is a convenience overload for the more general version taking a pair of iterators.
     *
     * @tparam ConatinerT
     *     container type
     *
     * @tparam PairT
     *     2-tuple-like type holding two floating-point values
     *
     * @tparam TX
     *     type of the <var>x</var> values
     *
     * @tparam TY
     *     type of the <var>y</var> values
     *
     * @tparam T
     *     type of the computed intercept and incline
     *
     * @param container
     *     container of data points
     *
     * @returns
     *     pair of intercept and incline
     *
     */
    template
    <
        typename ContainerT,
        typename PairT = typename ContainerT::value_type,
        typename TX = typename std::tuple_element<0, PairT>::type,
        typename TY = typename std::tuple_element<1, PairT>::type,
        typename T = std::common_type_t<TX, TY>,
        typename = std::enable_if_t<std::tuple_size<PairT>::value == 2 and std::is_floating_point_v<T>>
    >
    std::array<T, 2> linear_regression(const ContainerT& container)
    {
        return linear_regression(std::begin(container), std::end(container));
    }

}  // namespace msc

#define MSC_INCLUDED_FROM_REGRESSION_HXX
#include "regression.txx"
#undef MSC_INCLUDED_FROM_REGRESSION_HXX

#endif  // !defined(MSC_REGRESSION_HXX)
