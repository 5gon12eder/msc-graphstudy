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
 * @file numeric.hxx
 *
 * @brief
 *     Numeric mathematical operations.
 *
 */

#ifndef MSC_NUMERIC_HXX
#define MSC_NUMERIC_HXX

namespace msc
{

    /**
     * @brief
     *     Computes the finite integral over a range given as (<var>x</var>, <var>f</var>(<var>x</var>)) points.
     *
     * The data points do not have to equidistant.  They have to be sorted with respect to the <var>x</var>-values,
     * however.  If the range contains less than two data-points, the behavior is undefined.
     *
     * The complexity of this function is one pass over the range.
     *
     * @tparam FwdIterT
     *     forward iterator that (if dereferenced) must be destructurable to a pair of floating-point values
     *
     * @param first
     *     iterator pointing at the first data point
     *
     * @param last
     *     iterator pointing after the last data point
     *
     * @returns
     *     approximate integral
     *
     */
    template <typename FwdIterT>
    double integrate_trapezoidal(FwdIterT first, FwdIterT last) noexcept;

}  // namespace msc

#define MSC_INCLUDED_FROM_NUMERIC_HXX
#include "numeric.txx"
#undef MSC_INCLUDED_FROM_NUMERIC_HXX

#endif  // !defined(MSC_NUMERIC_HXX)
