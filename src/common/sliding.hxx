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
 * @file sliding.hxx
 *
 * @brief
 *     Sliding averages and such.
 *
 */

#ifndef MSC_SLIDING_HXX
#define MSC_SLIDING_HXX

#include <cassert>
#include <type_traits>
#include <utility>
#include <vector>

#include "stochastic.hxx"

namespace msc
{

    /**
     * @brief
     *     A Gaussian filter for a range of values.
     *
     * For a set of real values <var>x</var><sub>1</sub>, &hellip;, <var>x</var><sub><var>n</var></sub> a Gaussian
     * filter with width &sigma; is a callable object <var>g</var> that implements the univariate function
     * <var>f</var>(<var>x</var>) = &sum; <var>N</var>(<var>x</var>&minus;<var>x</var><sub><var>i</var></sub>) where
     * <var>N</var> is a Gaussian with center &mu; = 0 and width &sigma;.
     *
     * @tparam FwdIterT
     *     forward iterator type for the range of values
     *
     * @tparam ValueT
     *     floating-point type for the values
     *
     */
    template
    <
        typename FwdIterT,
        typename ValueT = typename std::iterator_traits<FwdIterT>::value_type,
        typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
    >
    class gaussian_kernel final
    {
    public:

        /**
         * @brief
         *     Instantiates a Gaussian kernel with filter width `sigma` for the range of values `[first, last)`.
         *
         * The behavior is undefined unless `sigma > 0.0`.
         *
         * @param first
         *     iterator to the first value in the range
         *
         * @param last
         *     iterator past the last value in the range
         *
         * @param sigma
         *     filter width
         *
         */
        gaussian_kernel(const FwdIterT first, const FwdIterT last, const ValueT sigma);

        /**
         * @brief
         *     Evaluates the kernel at the given point.
         *
         * @param x
         *     point at which to evaluate
         *
         * @returns
         *     <var>g</var>(<var>x</var>)
         *
         */
        ValueT operator()(const ValueT x) const;

    private:

        /** @brief Iterator to the first value in the range.  */
        FwdIterT _first{};

        /** @brief Iterator past the last value in the range.  */
        FwdIterT _last{};

        /** @brief Filter width.  */
        ValueT _sigma{};

    };  // class gaussian_kernel

    /**
     * @brief
     *     Evaluates the density of an event distribution over a regular grid.
     *
     * This function returns a list of `points` pairs `(x, y)` sorted by `x` where `x` is evenly spaced and ranges from
     * `minval` to `maxval` (both inclusive).  The values of `y` are determined by evaluating the `kernel` function at
     * `x`.  If `normalize == true`, then the values of `y` are additionally normalized such that the trapeziodal
     * integral over the range of points is 1 unless the function is zero everywhere in which case the result will be
     * zero, too.
     *
     * The behavior is undefined if the function is non-deterministic, non-steady or negative over the given interval.
     *
     * @tparam KernelT
     *     univariate callable `double -> double`
     *
     * @param kernel
     *     function to evaluate
     *
     * @param minval
     *     lowest value to consider
     *
     * @param maxval
     *     highest value to consider
     *
     * @param points
     *     number of evaluation points
     *
     * @param normalize
     *     whether the result shall be normalized
     *
     * @returns
     *     list of points
     *
     */
    template <typename KernelT>
    std::vector<std::pair<double, double>>
    make_density(const KernelT& kernel, double minval, double maxval, int points, bool normalize = true);

    /**
     * @brief
     *     Evaluates the density of an event distribution over an adaptive grid.
     *
     * This function returns a list of `points` pairs `(x, y)` sorted by `x` where `x` ranges from `minval` to `maxval`
     * (both inclusive).  The values of `y` are determined by evaluating the `kernel` function at `x`.  Evaluation
     * points are chosen adaptively such that they'll be more dense in regions where the function has a high curvature.
     * If `normalize == true`, then the values of `y` are additionally normalized such that the trapeziodal integral
     * over the range of points is 1 unless the function is zero everywhere in which case the result will be zero, too.
     *
     * The behavior is undefined if the function is non-deterministic, non-steady or negative over the given interval.
     *
     * @tparam KernelT
     *     univariate callable `double -> double`
     *
     * @param kernel
     *     function to evaluate
     *
     * @param minval
     *     lowest value to consider
     *
     * @param maxval
     *     highest value to consider
     *
     * @param normalize
     *     whether the result shall be normalized
     *
     * @returns
     *     list of points
     *
     */
    template <typename KernelT>
    std::vector<std::pair<double, double>>
    make_density_adaptive(const KernelT& kernel, double minval, double maxval, bool normalize = true);

    /**
     * @brief
     *     Computes the differential entropy of a probability density function.
     *
     * Note that differential entropy is not a direct measure of information content
     * (see also: https://en.wikipedia.org/wiki/Differential_entropy).
     *
     * The behavior is undefined if the density is not normalized.
     *
     * @param density
     *     pointwise probability density function
     *
     * @returns
     *     entropy of density
     *
     */
    double get_differential_entropy_of_pdf(const std::vector<std::pair<double, double>>& density);

}  // namespace msc

#define MSC_INCLUDED_FROM_SLIDING_HXX
#include "sliding.txx"
#undef MSC_INCLUDED_FROM_SLIDING_HXX

#endif  // !defined(MSC_SLIDING_HXX)
