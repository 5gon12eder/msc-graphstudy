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
 * @file histogram.hxx
 *
 * @brief
 *     Histograms.
 *
 */

#ifndef MSC_HISTOGRAM_HXX
#define MSC_HISTOGRAM_HXX

#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "enums/binnings.hxx"
#include "stochastic.hxx"

namespace msc
{

    /**
     * @brief
     *     A dense histogram.
     *
     * The histogram is initialized by its constructor from a range of data (events).  The individual events are not
     * recoded internally but the binned frequency data is.
     *
     */
    class histogram final
    {
    private:

        /**
         * @brief
         *     Initializes a histogram in an empty state.
         *
         * This constructor shall only be used to implement the move operations.
         *
         */
        histogram() noexcept;

    public:

        /**
         * @brief
         *     Initializes a histogram from a range of data (events) using automatically selected bin width.
         *
         * The range of data is iterated over twice.  The behavior is undefined if the number of events is less than
         * three.
         *
         * @tparam FwdIterT
         *     forward iterator type that can be dereferenced to a floating-point type
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param first
         *     begin of the range of events
         *
         * @param last
         *     end of the range of events
         *
         */
        template
        <
          typename FwdIterT,
          typename ValueT = typename std::iterator_traits<FwdIterT>::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(FwdIterT first, FwdIterT last);

        /**
         * @brief
         *     Initializes a histogram from a container of events using automatically selected bin width.
         *
         * The container is iterated over twice.  The behavior is undefined if the number of events is less than three.
         *
         * @tparam ContainerT
         *     container type that holds floating-point types
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param events
         *     container of events
         *
         */
        template
        <
          typename ContainerT,
          typename ValueT = typename ContainerT::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(const ContainerT& events);

        /**
         * @brief
         *     Initializes a histogram from a range of data (events) using an explicit number of bins.
         *
         * The range of data is iterated over twice.  The behavior is undefined if the number of events is less than
         * three or the specified number of bins is not positive.
         *
         * @tparam FwdIterT
         *     forward iterator type that can be dereferenced to a floating-point type
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param first
         *     begin of the range of events
         *
         * @param last
         *     end of the range of events
         *
         * @param bincount
         *     desired number of bins
         *
         */
        template
        <
          typename FwdIterT,
          typename ValueT = typename std::iterator_traits<FwdIterT>::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(FwdIterT first, FwdIterT last, std::size_t bincount);

        /**
         * @brief
         *     Initializes a histogram from a container of events using an explicit number of bins.
         *
         * The container is iterated over twice.  The behavior is undefined if the number of events is less than
         * three or the specified number of bins is not positive.
         *
         * @tparam ContainerT
         *     container type that holds floating-point types
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param events
         *     container of events
         *
         * @param bincount
         *     desired number of bins
         *
         */
        template
        <
          typename ContainerT,
          typename ValueT = typename ContainerT::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(const ContainerT& events, std::size_t bincount);

        /**
         * @brief
         *     Initializes a histogram from a range of data (events) using an explicit bin width.
         *
         * The range of data is iterated over twice.  The behavior is undefined if the number of events is less than
         * three or the specified bin width is not positive.
         *
         * @tparam FwdIterT
         *     forward iterator type that can be dereferenced to a floating-point type
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param first
         *     begin of the range of events
         *
         * @param last
         *     end of the range of events
         *
         * @param binwidth
         *     desired bin width
         *
         */
        template
        <
          typename FwdIterT,
          typename ValueT = typename std::iterator_traits<FwdIterT>::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(FwdIterT first, FwdIterT last, double binwidth);

        /**
         * @brief
         *     Initializes a histogram from a range of data (events) using an explicit bin width.
         *
         * The range of data is iterated over twice.  The behavior is undefined if the number of events is less than
         * three or the specified bin width is not positive.
         *
         * @tparam ContainerT
         *     container type that holds floating-point types
         *
         * @tparam ValueT
         *     floating-point type of the event data
         *
         * @param events
         *     container of events
         *
         * @param binwidth
         *     desired bin width
         *
         */
        template
        <
          typename ContainerT,
          typename ValueT = typename ContainerT::value_type,
          typename = std::enable_if_t<std::is_floating_point_v<ValueT>>
        >
        explicit histogram(const ContainerT& events, double binwidth);

        /**
         * @brief
         *     Creates a new histogram that contains a copy of this histogram's data.
         *
         * @param other
         *     histogram to copy
         *
         */
        histogram(const histogram& other) = default;

        /**
         * @brief
         *     Makes this histogram contain a copy of another histogram's data.
         *
         * @param other
         *     histogram to copy
         *
         */
        histogram& operator=(const histogram& other) = default;

        /**
         * @brief
         *     Creates a new histogram that contains another histogram's data.
         *
         * After the operation, the other histogram will be in a valid but unspecified state.
         *
         * @param other
         *     histogram to obtain the data from
         *
         */
        histogram(histogram&& other) noexcept;

        /**
         * @brief
         *     Makes this histogram contain another histogram's data.
         *
         * After the operation, the other histogram will be in a valid but unspecified state.
         *
         * @param other
         *     histogram to obtain the data from
         *
         */
        histogram& operator=(histogram&& other) noexcept;

        /**
         * @brief
         *     Returns the number of events that were used to populate the histogram.
         *
         * @returns
         *     number of events
         *
         */
        std::size_t size() const noexcept
        {
            return _summary.count;
        }

        /**
         * @brief
         *     Returns the number of bins in the histogram.
         *
         * @returns
         *     number of bins
         *
         */
        std::size_t bincount() const noexcept
        {
            return _frequencies.size();
        }

        /**
         * @brief
         *     Returns the width of each bin in the histogram.
         *
         * @returns
         *     bin width
         *
         */
        double binwidth() const noexcept
        {
            return _binwidth;
        }

        /**
         * @brief
         *     Returns the minimum of the events that were used to populate the histogram.
         *
         * @returns
         *     smallest event
         *
         */
        double min() const noexcept
        {
            return _summary.min;
        }

        /**
         * @brief
         *     Returns the maximum of the events that were used to populate the histogram.
         *
         * @returns
         *     largest event
         *
         */
        double max() const noexcept
        {
            return _summary.max;
        }

        /**
         * @brief
         *     Returns the mean of the events that were used to populate the histogram.
         *
         * @returns
         *     arithmetic men of the events
         *
         */
        double mean() const noexcept
        {
            return _summary.mean;
        }

        /**
         * @brief
         *     Returns the root mean squared of the events that were used to populate the histogram.
         *
         * @returns
         *     RMS of the events
         *
         */
        double rms() const noexcept
        {
            return _summary.rms;
        }

        /**
         * @brief
         *     Returns the entropy of the histogram.
         *
         * The entropy is computed by summing up the weighted (by frequency) negative binary logarithms of the bin
         * values (frequencies).  Empty bins (for which the logarithm diverges) are counted as zero.
         *
         * @returns
         *     entropy in bits
         *
         */
        double entropy() const noexcept
        {
            return _entropy;
        }

        /**
         * @brief
         *     Returns the center (abscissa) of the bin with the given index.
         *
         * The behavior is undefined unless `0 <= idx < bincount()`.
         *
         * @param idx
         *     index of the bin
         *
         * @returns
         *     center of bin `idx`
         *
         */
        double center(std::size_t idx) const;

        /**
         * @brief
         *     Returns the values (frequency) of the bin with the given index.
         *
         * The behavior is undefined unless `0 <= idx < bincount()`.
         *
         * @param idx
         *     index of the bin
         *
         * @returns
         *     relative frequency in bin `idx`
         *
         */
        double frequency(std::size_t idx) const;

        /**
         * @brief
         *     Returns a constant reference to the internal bin array.
         *
         * @returns
         *     constant reference to bin array
         *
         */
        const std::vector<double>& frequencies() const noexcept
        {
            return _frequencies;
        }

        /**
         * @brief
         *     Returns the binning strategy that was used for this histogram.
         *
         * @returns
         *     binning strategy
         *
         */
        binnings binning() const noexcept
        {
            return _binning;
        }

        /**
         * @brief
         *     Exchanges data between two histograms.
         *
         * @param lhs
         *     first histogram
         *
         * @param rhs
         *     second histogram
         *
         */
        friend void swap(histogram& lhs, histogram& rhs) noexcept
        {
            using std::swap;
            swap(lhs._frequencies, rhs._frequencies);
            swap(lhs._summary,     rhs._summary);
            swap(lhs._start,       rhs._start);
            swap(lhs._binwidth,    rhs._binwidth);
            swap(lhs._entropy,     rhs._entropy);
            swap(lhs._binning,     rhs._binning);
        }

    private:

        /** @brief Values of the histogram bins.  */
        std::vector<double> _frequencies{};

        /** @brief Stochastic summary obtained along the way.  */
        stochastic_summary _summary{nullptr};

        /** @brief Lower bound of the analyzed data (no necessarily the minimum).  */
        double _start{std::numeric_limits<double>::quiet_NaN()};

        /** @brief Histogram bin width..  */
        double _binwidth{std::numeric_limits<double>::quiet_NaN()};

        /** @brief Entropy of the histogram.  */
        double _entropy{std::numeric_limits<double>::quiet_NaN()};

        /** @brief Method that was used to choose the bin width / count.  */
        binnings _binning{};

#ifndef MSC_PARSED_BY_DOXYGEN

        template <typename FwdIterT>
        double _init_histo(FwdIterT first, FwdIterT last);

#endif  // MSC_PARSED_BY_DOXYGEN

    };  // class histogram

    /**
     * @brief
     *     Returns the suggested binwidth according to Scott's normal reference rule.
     *
     * The behavior is undefined unless `n > 0` and `stdev > 0.0`.
     *
     * https://en.wikipedia.org/wiki/Histogram#Scott.27s_normal_reference_rule
     *
     * @param n
     *     number of observed events
     *
     * @param stdev
     *     observed standard deviation
     *
     * @returns
     *     recommended bin width
     *
     */
    double binwidth_scott_normal_reference(std::size_t n, double stdev);

}  // namespace msc

#define MSC_INCLUDED_FROM_HISTOGRAM_HXX
#include "histogram.txx"
#undef MSC_INCLUDED_FROM_HISTOGRAM_HXX

#endif  // !defined(MSC_HISTOGRAM_HXX)
