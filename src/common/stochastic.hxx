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
 * @file stochastic.hxx
 *
 * @brief
 *     Common stochasitc functions.
 *
 */

#ifndef MSC_STOCHASTIC_HXX
#define MSC_STOCHASTIC_HXX

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include <type_traits>
#include <utility>

#include "math_constants.hxx"
#include "useful.hxx"  // for square

namespace msc
{

    /**
     * @brief
     *     Returns the minimum and maximum from a non-empty range of values.
     *
     * The behavior is undefined if `first == last`.
     *
     * @tparam FwdIterT
     *     forward iterator type designating the range of values
     *
     * @tparam ValueT
     *     arithmetic type of the values
     *
     * @param first
     *     iterator pointing to the first value in the range
     *
     * @param last
     *     iterator pointing past the last value in the range
     *
     * @returns
     *     pair of minimum and maximum value (in that order)
     *
     */
    template <typename FwdIterT, typename ValueT = typename std::iterator_traits<FwdIterT>::value_type>
    std::enable_if_t<std::is_arithmetic_v<ValueT>, std::pair<ValueT, ValueT>>
    min_max(const FwdIterT first, const FwdIterT last)
    {
        assert(first != last);
        const auto [minit, maxit] = std::minmax_element(first, last);
        assert((minit != last) && (maxit != last));
        return {*minit, *maxit};
    }

    /**
     * @brief
     *     Returns the minimum and maximum from a non-empty container of values.
     *
     * The behavior is undefined if the container is empty.
     *
     * This is a convenience overload for the version that takes a pair of iterators.
     *
     * @tparam ContainerT
     *     container type
     *
     * @tparam ValueT
     *     arithmetic type of the values
     *
     * @param container
     *     container holding the values
     *
     * @returns
     *     pair of minimum and maximum value (in that order)
     *
     */
    template <typename ContainerT, typename ValueT = typename ContainerT::value_type>
    std::enable_if_t<std::is_arithmetic_v<ValueT>, std::pair<ValueT, ValueT>>
    min_max(const ContainerT& container)
    {
        return min_max(std::begin(container), std::end(container));
    }

    /**
     * @brief
     *     Returns the arithmetic mean and standard deviation of a range of three or more values.
     *
     * The behavior is undefined if `std::distance(first, last) < 3`.
     *
     * @tparam FwdIterT
     *     forward iterator type designating the range of values
     *
     * @tparam ValueT
     *     arithmetic type of the values
     *
     * @param first
     *     iterator pointing to the first value in the range
     *
     * @param last
     *     iterator pointing past the last value in the range
     *
     * @returns
     *     arithmetic mean and standard deviation (in that order)
     *
     */
    template <typename FwdIterT, typename ValueT = typename std::iterator_traits<FwdIterT>::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, std::pair<ValueT, ValueT>>
    mean_stdev(const FwdIterT first, const FwdIterT last)
    {
        constexpr auto zero = ValueT{};
        const auto n = std::distance(first, last);
        assert(n >= 3);
        const auto mean = std::accumulate(first, last, zero) / n;
        const auto lambda = [mean](auto acc, auto x){ return acc + ::msc::square(x - mean); };
        const auto stdev = std::sqrt(std::accumulate(first, last, zero, lambda) / (n - 1));
        return {mean, stdev};
    }

    /**
     * @brief
     *     Returns the arithmetic mean and standard deviation of a container of three or more values.
     *
     * The behavior is undefined if `container.size() < 3`.
     *
     * This is a convenience overload for the version that takes a pair of iterators.
     *
     * @tparam ContainerT
     *     container type
     *
     * @tparam ValueT
     *     arithmetic type of the values
     *
     * @param container
     *     container holding the values
     *
     * @returns
     *     arithmetic mean and standard deviation (in that order)
     *
     */
    template <typename ContainerT, typename ValueT = typename ContainerT::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, std::pair<ValueT, ValueT>>
    mean_stdev(const ContainerT& container)
    {
        return mean_stdev(std::begin(container), std::end(container));
    }

    /**
     * @brief
     *     Aggregation of various elementary stochastic properties or a range <var>x</var><sub>1</sub>, &hellip;
     *     <var>x</var><sub><var>n</var></sub> of real values.
     *
     */
    struct stochastic_summary final
    {

        /** @brief Number of values <var>n</var>.  */
        std::size_t count{};

        /** @brief Smallest value min { <var>x</var><sub><var>i</var></sub> }.  */
        double min{};

        /** @brief Largets value max { <var>x</var><sub><var>i</var></sub> }.  */
        double max{};

        /** @brief Arithmetic mean <var>n</var><sup>&minus;1</sup> &sum; <var>x</var><sub><var>i</var></sub>.  */
        double mean{};

        /**
         * @brief
         *     Root mean squared
         *     ( <var>n</var><sup>&minus;1</sup> &sum; <var>x</var><sub><var>i</var></sub><sup>2</sup> )<sup>1/2</sup>.
         *
         */
        double rms{};

        /**
         * @brief
         *     Initializes a `stochastic_summary` with all values set to zero.
         *
         */
        constexpr stochastic_summary() noexcept = default;

        /**
         * @brief
         *     Initializes a `stochastic_summary` with the provided values.
         *
         * @param count
         *     number of values
         *
         * @param min
         *     minimum value
         *
         * @param max
         *     maximum value
         *
         * @param mean
         *     arithmetic mean
         *
         * @param rms
         *     root mean squared
         *
         */
        constexpr stochastic_summary(const std::size_t count,
                                     const double min,
                                     const double max,
                                     const double mean,
                                     const double rms) noexcept :
            count{count},
            min{min},
            max{max},
            mean{mean},
            rms{rms}
        {
        }

        /**
         * @brief
         *     Initializes a `stochastic_summary` with all values set to NaNs.
         *
         */
        constexpr stochastic_summary(const std::nullptr_t) noexcept :
            min{std::numeric_limits<double>::quiet_NaN()},
            max{std::numeric_limits<double>::quiet_NaN()},
            mean{std::numeric_limits<double>::quiet_NaN()},
            rms{std::numeric_limits<double>::quiet_NaN()}
        {
        }

        /**
         * @brief
         *     Returns the sample standard deviation computed from the mean and RMS.
         *
         * The behavior is undefined unless the result is mathematically well-defined.
         *
         * @returns
         *     sample standard deviation
         *
         */
        double stdev() const noexcept
        {
            assert(this->count > 1);
            assert(std::abs(this->rms) >= std::abs(this->mean));
            const auto n = static_cast<double>(this->count);
            return std::sqrt(n / (n - 1.0) * (square(this->rms) - square(this->mean)));
        }

        /**
         * @brief
         *     Returns the population standard deviation computed from the mean and RMS.
         *
         * The behavior is undefined unless the result is mathematically well-defined.
         *
         * @returns
         *     population standard deviation
         *
         */
        double stdevp() const noexcept
        {
            assert(std::abs(this->rms) >= std::abs(this->mean));
            return std::sqrt(square(this->rms) - square(this->mean));
        }

    };  // struct stochastic_summary

    /**
     * @brief
     *     Computes numer of events, minumum, maximum, mean and RMS of a population given by a pair of forward
     *     iterators.
     *
     * The range is iterated only once and each iterator is dereferenced only once, which is preferable if iterating is
     * non-trivial.
     *
     * If the range is empty, the behavior is undefined.
     *
     * @tparam FwdIterT
     *     forward iterator type that can be dereferenced to a floating-point value
     *
     * @param first
     *     begin of the range
     *
     * @param last
     *     end of the range
     *
     * @returns
     *     basic stochastic properties of the population
     *
     */
    template <typename FwdIterT, typename ValueT = typename std::iterator_traits<FwdIterT>::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, stochastic_summary>
    get_stochastic_summary(const FwdIterT first, const FwdIterT last)
    {
        auto sum = ValueT{};
        auto sqrsum = ValueT{};
        auto min = std::numeric_limits<ValueT>::max();
        auto max = std::numeric_limits<ValueT>::min();
        auto count = std::size_t{};
        for (auto it = first; it != last; ++it) {
            const auto value = *it;
            sum += value;
            sqrsum += square(value);
            min = std::min(min, value);
            max = std::max(max, value);
            count += 1;
        }
        return {count, min, max, sum / count, std::sqrt(sqrsum / count)};
    }

    /**
     * @brief
     *     Computes numer of events, minumum, maximum, mean and RMS of a population given by a container.
     *
     * This is a not-so-useful convenience overload of the function that takes a pair of iterators.
     *
     * If the range is empty, the behavior is undefined.
     *
     * @tparam ContainerT
     *     container type holding floating-point values
     *
     * @param container
     *     container of events
     *
     * @returns
     *     basic stochastic properties of the population
     *
     */
    template <typename ContainerT, typename ValueT = typename ContainerT::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, stochastic_summary>
    get_stochastic_summary(const ContainerT& container)
    {
        return get_stochastic_summary(std::begin(container), std::end(container));
    }

    /**
     * @brief
     *     Computes the discrete entropy of a range of frequencies.
     *
     * The behavior is undefined unless the range is empty or all values are non-negative and sum up to 1.
     *
     * @tparam FwdIterT
     *     forward iterator type
     *
     * @tparam ValueT
     *     floating-point type
     *
     * @param first
     *     iterator pointing to the first frequency
     *
     * @param last
     *     iterator pointing past the last frequency
     *
     * @returns
     *     discrete entropy in bits
     *
     */
    template <typename FwdIterT, typename ValueT = typename std::iterator_traits<FwdIterT>::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, ValueT>
    entropy(const FwdIterT first, const FwdIterT last)
    {
        constexpr auto zero = ValueT{0};
        assert(std::all_of(first, last, [](const auto x){ return (x >= zero); }));
        if (first == last) {
            return zero;
        }
        assert(std::abs(1.0 - std::accumulate(first, last, zero)) < 1.0e-6);
        const auto lambda = [](const auto acc, const auto x){
            return (x > zero) ? acc + x * std::log2(x) : acc;
        };
        // We use `std::abs` rather than negation because we never want -0 as a result.
        return std::abs(std::accumulate(first, last, zero, lambda));
    }

    /**
     * @brief
     *     Computes the discrete entropy of a container of frequencies.
     *
     * The behavior is undefined unless the container is empty or all values are non-negative and sum up to 1.
     *
     * This is a convenience overload for the function that takes a pair of iterators.
     *
     * @tparam ContainerT
     *     container type
     *
     * @tparam ValueT
     *     floating-point type
     *
     * @param container
     *     container of frequencies
     *
     * @returns
     *     discrete entropy in bits
     *
     */
    template <typename ContainerT, typename ValueT = typename ContainerT::value_type>
    std::enable_if_t<std::is_floating_point_v<ValueT>, ValueT>
    entropy(const ContainerT& container)
    {
        return entropy(std::begin(container), std::end(container));
    }

    /**
     * @brief
     *     Function object implementing a Gaussian function.
     *
     */
    struct gaussian final
    {

        /** @brief Initializes a standard (&mu; = 0, &sigma; = 1) Gaussian function.  */
        gaussian() noexcept = default;

        /**
         * @brief
         *     Initializes a Gaussian function with the provided parameters.
         *
         * The behavior is undefined unless `sigma > 0.0`.
         *
         * @param mu
         *     expectation value
         *
         * @param sigma
         *     square root of the variance
         *
         */
        gaussian(const double mu, const double sigma) : _mu{mu}, _var{sigma * sigma}
        {
            assert(sigma > 0.0);
        }

        /**
         * @brief
         *     Returns the value of the function at the specified point.
         *
         * @param x
         *     point at which to evaluate
         *
         * @returns
         *     function value at `x`
         *
         */
        double operator()(const double x) const noexcept
        {
            const auto dx = x - _mu;
            return 1.0 / std::sqrt(2.0 * M_PI * _var) * std::exp(-(dx * dx) / (2.0 * _var));
        }

    private:

        /** @brief Expectation value.  */
        double _mu{0.0};

        /** @brief Variance.  */
        double _var{1.0};

    };  // struct gaussian

}  // namespace msc

#endif  // !defined(MSC_STOCHASTIC_HXX)
