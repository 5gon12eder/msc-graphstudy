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
 * @file data_analysis.hxx
 *
 * @brief
 *     Generic data analysis and presentation.
 *
 */

#ifndef MSC_DATA_ANALYSIS_HXX
#define MSC_DATA_ANALYSIS_HXX

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "enums/kernels.hxx"
#include "file.hxx"

namespace msc
{

    class histogram;
    struct json_object;
    struct stochastic_summary;

    /**
     * @brief
     *     Generic data analyzer and presenter.
     *
     * Instances of this class store parameters *how* the data should be analyzed but not the data itself.  They can be
     * used to analyze multiple sources of data in sequence, eventually changing or tuning the parameters in between.
     *
     */
    class data_analyzer final
    {
    public:

        /**
         * @brief
         *     Creates a new data analyzer that will use the given kernel.
         *
         * The behavior is undefined if `kern` is not a declared enumerator.
         *
         * @param kern
         *     kernel to use (can be changed later)
         *
         */
        data_analyzer(const kernels kern) noexcept;

        /**
         * @brief
         *     Analyzes a range of data.
         *
         * This function proceeds as follows (using <var>n</var> for illustration purpose as if determined by
         * `const auto n = std::distance(first, last);` which is never evaluated, though):
         *
         * 1. If there are less than three events, it immediately returns `false` and has no effect.  This step
         *    (which is always performed) exercises at most 3 iterator increments.
         * 2. Otherwise, if `get_kernel()` is `kernels::raw`, aggregates all events into a vector and outputs it to the
         *    specified destination as raw list of numbers in a format Gnuplot can understand with a commented header at
         *    the top of the file (see `#write_events`).  This step exercises exactly <var>n</var> iterator increments.
         * 3. Otherwise, if `get_kernel()` is `kernels::boxed`, a `msc::histogram` from the range `[first, last)` will
         *    be constructed.  If `get_width().has_value() && !get_bins().has_value()` then `#get_width().value()` will
         *    be used as fixed bin width.  Likewise, if `!get_width().has_value() && get_bins().has_value()` then
         *    `#get_bins().value()` will be used as fixed bin count.  Otherwise, if
         *    `!get_width().has_value() && !get_bins().has_value()`, then the bin width will be chosen according to the
         *    heuristic implemented in the two-argument constructor of the `histogram` class.  Otherwise, if
         *    `get_width().has_value() && get_bins().has_value()` a `std::invalid_argument` error is thrown.  In either
         *    case, exactly 2 &sdot; <var>n</var> iterator increments will be performed.  The binned frequency data will
         *    then be written to the specified destination (see `#write_frequencies`).
         * 4. Otherwise, if `get_kernel()` is `kernels::gaussian`, a smooth density with a gaussian kernel and parameter
         *    <var>&sigma;</var> is computed and evaluated at <var>m</var> points over the range
         *    [<var>x</var><sub>min</sub>, <var>x</var><sub>max</sub>] which is chosen as if by `const auto xmin =
         *    get_lower().value_or(x1); const auto xmax = get_upper().value_or(x2);` where <var>x</var><sub>1</sub> and
         *    <var>x</var><sub>2</sub> refer to the smallest and largest event in the data set respectively.  If this
         *    results in an invalid range (<var>x</var><sub>min</sub> &gt; <var>x</var><sub>max</sub>), `false` is
         *    returned immediately and nothing further will be done.  If `get_points().has_value()`.  <var>&sigma;</var>
         *    is chosen as if by `const auto sigma = get_width().value_or(other.value_or(scott)) / 2.0;` where `other`
         *    is an optional holding the desired range divided by the desired number of bins if this value exists and
         *    `scott` refers to the bin width that would be chosen for the data accordin to Scott's normal reference
         *    rule.  If `points().has_value()` then the interpolation will be evaluated at thta many equidistant points.
         *    Otherwise, an adaptive strategy is used.  Finally, the interpolated density will be written to the
         *    specified destination (see `#write_density`).  This step will perform up to <var>n</var> &sdot;
         *    (<var>m</var> + 1) iterator increments.  (It might try to use less if enough memory is available for
         *    caching.)
         * 5. Otherwise, the behavior is undefined.
         * 6. Finally, if any data was processed, the `info` object will be augmented with a `size` (which is set to
         *    <var>n</var>), `minimum`, `maximum`, `mean` and `rms` attribute.  The `subifno` object with a `filename`
         *    and maybe other attributes specific to the applied kernel.
         * 7. If all this is done, `true` is returned.
         *
         * @tparam FwdIterT
         *     forward iterator type
         *
         * @param first
         *     iterator pointing to the first event to consider
         *
         * @param last
         *     iterator pointing after the last event to consider
         *
         * @param info
         *     meta-data object to augment with information
         *
         * @param subinfo
         *     meta-data object to augment with information
         *
         * @returns
         *     whether the data was successfully analyzed and the meta-data augmented
         *
         */
        template <typename FwdIterT>
        [[nodiscard]] bool
        analyze_oknodo(const FwdIterT first, const FwdIterT last, json_object& info, json_object& subinfo) const;

        /**
         * @brief
         *     Analyzes a container of data.
         *
         * This is a convenience overload for the more general function that takes a pair of iterators.  All remarks
         * about that function apply here, too.
         *
         * @tparam ContainerT
         *     iterable type with an iterator that satisfies at least the *ForwardIterator* concept
         *
         * @param sample
         *     container of event data to analyze
         *
         * @param info
         *     meta-data object to augment with information
         *
         * @param subinfo
         *     meta-data object to augment with information
         *
         * @returns
         *     whether the data was successfully analyzed and the meta-data augmented
         *
         */
        template <typename ContainerT>
        [[nodiscard]] bool analyze_oknodo(const ContainerT& sample, json_object& info, json_object& subinfo) const;

        /**
         * @brief
         *     Analyzes a range of data and throws an exception if not enough data is available.
         *
         * This function is semantically equivalent to calling `analyze_oknodo(first, last, info)` and throwing an
         * exception if it returns `false`.
         *
         * @tparam FwdIterT
         *     forward iterator type
         *
         * @param first
         *     iterator pointing to the first event to consider
         *
         * @param last
         *     iterator pointing after the last event to consider
         *
         * @param info
         *     meta-data object to augment with information
         *
         * @param subinfo
         *     meta-data object to augment with information
         *
         * @throws std::logic_error
         *     if no analysis could be performed
         *
         */
        template <typename FwdIterT>
        void analyze(const FwdIterT first, const FwdIterT last, json_object& info, json_object& subinfo) const;

        /**
         * @brief
         *     Analyzes a container of data and throws an exception if not enough data is available.
         *
         * This function is semantically equivalent to calling `analyze_oknodo(sample, info)` and throwing an exception
         * if it returns `false`.
         *
         * @tparam ContainerT
         *     iterable type with an iterator that satisfies at least the *ForwardIterator* concept
         *
         * @param sample
         *     container of event data to analyze
         *
         * @param info
         *     meta-data object to augment with information
         *
         * @param subinfo
         *     meta-data object to augment with information
         *
         * @throws std::logic_error
         *     if no analysis could be performed
         *
         */
        template <typename ContainerT>
        void analyze(const ContainerT& sample, json_object& info, json_object& subinfo) const;

        /**
         * @brief
         *     Returns the currenly selected kernel.
         *
         * @returns
         *     currently selected kernel
         *
         */
        kernels get_kernel() const noexcept;

        /**
         * @brief
         *     Sets the selected kernel.
         *
         * @param kern
         *     desired kernel
         *
         */
        void set_kernel(kernels kern);

        /**
         * @brief
         *     Returns the currenly selected lower bound (if any).
         *
         * @returns
         *     currently selected lower bound
         *
         */
        std::optional<double> get_lower() const noexcept;

        /**
         * @brief
         *     Returns the currenly selected upper bound (if any).
         *
         * @returns
         *     currently selected upper bound
         *
         */
        std::optional<double> get_upper() const noexcept;

        /**
         * @brief
         *     Clears the selected range.
         *
         * This convenience function is equivalent to `set_range(std::nullopt, std::nullopt)`.
         *
         */
        void set_range() noexcept;

        /**
         * @brief
         *     Sets (or clears) the upper and lower bounds of the selected range.
         *
         * The behavior is undefined if either bound is non-finite or if
         * `lower.value_or(DBL_MIN) <= upper.value_or(DBL_MAX)` does not hold.
         *
         * Note that setting the lower bound to `DBL_MIN` or the upper bound to `DBL_MAX` would be extremely unwise and
         * have vastly different semantics than setting either bound to `std::nullopt` (which you'd probably want to do
         * instead).
         *
         * @param lower
         *     desired lower bound (if any)
         *
         * @param upper
         *     desired upper bound (if any)
         *
         */
        void set_range(std::optional<double> lower, std::optional<double> upper);

        /**
         * @brief
         *     Returns the currenly selected kernel width (if any).
         *
         * @returns
         *     currently selected kernel width
         *
         */
        std::optional<double> get_width() const noexcept;

        /**
         * @brief
         *     Sets (or clears) the selected kernel width.
         *
         * The behavior is undefined unless `width.has_value()` implies `std::isfinite(width.value())` and
         * `width.value() > 0.0`.
         *
         * @param width
         *     desired kernel width
         *
         */
        void set_width(std::optional<double> width = std::nullopt);

        /**
         * @brief
         *     Returns the currenly selected number of bins (if any).
         *
         * @returns
         *     currently selected number of bins
         *
         */
        std::optional<int> get_bins() const noexcept;

        /**
         * @brief
         *     Sets (or clears) the selected number of bins.
         *
         * The behavior is undefined unless `bins.has_value()` implies `bins.value() > 0`.
         *
         * @param bins
         *     desired number of bins
         *
         */
        void set_bins(std::optional<int> bins = std::nullopt);

        /**
         * @brief
         *     Returns the currenly selected number of evaluation points (if any).
         *
         * @returns
         *     currently selected number of evaluation points
         *
         */
        std::optional<int> get_points() const noexcept;

        /**
         * @brief
         *     Sets (or clears) the currenly selected number of evaluation points.
         *
         * The behavior is undefined unless `points.value_or(default_points_min) > 0` holds.
         *
         * @param points
         *     desired number of evaluation points
         *
         */
        void set_points(std::optional<int> points = std::nullopt);

        /**
         * @brief
         *     Returns the currently selected output destination.
         *
         * @returns
         *     currently selected output destination
         *
         */
        const output_file& get_output() const noexcept;

        /**
         * @brief
         *     Sets the currently selected output destination.
         *
         * @returns
         *     desired output destination
         *
         */
        void set_output(output_file dst);

    private:

        /** @brief Kernel to use for analysis.  */
        kernels _kernel{};

        /** @brief Lower end of the range of events to consider.  */
        std::optional<double> _lower{};

        /** @brief Upper end of the range of events to consider.  */
        std::optional<double> _upper{};

        /** @brief Histogram bin width or Gaussian filter width.  */
        std::optional<double> _width{};

        /** @brief Histogram bin count.  */
        std::optional<int> _bins{};

        /** @brief Sliding average evaluation points (non-adaptive).  */
        std::optional<int> _points{};

        /** @brief Data output file.  */
        output_file _output{};

#ifndef MSC_PARSED_BY_DOXYGEN

        void _update_info(json_object& info, json_object& subinfo, const histogram& histo) const;

        void _update_info(json_object& info,
                          json_object& subinfo,
                          const stochastic_summary& summary,
                          std::optional<double> sigma = std::nullopt,
                          std::optional<std::size_t> points = std::nullopt,
                          std::optional<double> entropy = std::nullopt,
                          std::optional<std::pair<double, double>> maxelm = std::nullopt) const;

        void _update_info_common(json_object& info, json_object& subinfo) const;

#endif  // MSC_PARSED_BY_DOXYGEN

    };  // class data_analyzer

    /**
     * @brief
     *     Returns an object that is ready to use as collector for entropy data.
     *
     * @returns
     *     empty vector of appropriate type
     *
     */
    std::vector<std::pair<double, double>> initialize_entropies();

    /**
     * @brief
     *     Appends the entropy of an analysis.
     *
     * @param entropies
     *     data structire to append to
     *
     * @param info
     *     data structure to extract information from
     *
     * @param keyname
     *     name of the independent variable in `info`
     *
     * @param valname
     *     name of the dependent variable in `info`
     *
     */
    void append_entropy(std::vector<std::pair<double, double>>& entropies,
                        const json_object& info,
                        std::string_view keyname,
                        std::string_view valname = "entropy");

    /**
     * @brief
     *     Performs a linear regression over the data held in `entropies` and assigns the result as `entropy-intercept`
     *     and `entropy-slope` to `info`.
     *
     * If `entropies.empty()` this function has no effect.
     *
     * @param entropies
     *     collected data points
     *
     * @param info
     *     data structure to assign to
     *
     */
    void assign_entropy_regression(const std::vector<std::pair<double, double>>& entropies, json_object& info);

}  // namespace msc

/// @cond FALSE
#define MSC_INCLUDED_FROM_DATA_ANALYSIS_HXX
#include "data_analysis.txx"
#undef MSC_INCLUDED_FROM_DATA_ANALYSIS_HXX
/// @endcond

#endif  // !defined(MSC_DATA_ANALYSIS_HXX)
