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

#ifndef MSC_INCLUDED_FROM_DATA_ANALYSIS_HXX
#  error "Never `#include <data_analysis.txx>` directly; `#include <data_analysis.hxx>` instead"
#endif

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <utility>
#include <vector>

#include "histogram.hxx"
#include "io.hxx"
#include "sliding.hxx"
#include "stochastic.hxx"
#include "useful.hxx"

namespace msc
{

    namespace detail::data_analysis
    {

        template <typename FwdIterT>
        constexpr bool distance_less_than_three(const FwdIterT first, const FwdIterT last) noexcept
        {
            // We could implement this function as `return std::distance(first, last) < 3;` but it would be an O(N)
            // operation which we cannot afford for large N.
            auto tally = 0;
            for (auto it = first; it != last; ++it) {
                if (++tally == 3) {
                    return false;
                }
            }
            return true;
        }

    }  // namespace detail::data_analysis

    inline data_analyzer::data_analyzer(const kernels kern) noexcept : _kernel{kern}
    {
        assert(kern != kernels{});
    }

    template <typename ContainerT>
    void data_analyzer::analyze(const ContainerT& sample, json_object& info, json_object& subinfo) const
    {
        if (!this->analyze_oknodo(sample, info, subinfo)) {
            throw std::logic_error{"Not enough data for a statistical analysis"};
        }
    }

    template <typename FwdIterT>
    void data_analyzer::analyze(const FwdIterT first,
                                const FwdIterT last,
                                json_object& info,
                                json_object& subinfo) const
    {
        if (!this->analyze_oknodo(first, last, info, subinfo)) {
            throw std::logic_error{"Not enough data for a statistical analysis"};
        }
    }

    template <typename ContainerT>
    [[nodiscard]] bool data_analyzer::analyze_oknodo(const ContainerT& sample,
                                                     json_object& info,
                                                     json_object& subinfo) const
    {
        return this->analyze_oknodo(std::begin(sample), std::end(sample), info, subinfo);
    }

    template <typename FwdIterT>
    [[nodiscard]] bool data_analyzer::analyze_oknodo(const FwdIterT first,
                                                     const FwdIterT last,
                                                     json_object& info,
                                                     json_object& subinfo) const
    {
        // TODO: It would be so much better to split this into a switch statement with a single return per case that
        //       calls a dedicated subroutine to perform the actual work.
        if (detail::data_analysis::distance_less_than_three(first, last)) {
            return false;
        } else if (_kernel == kernels::boxed) {
            const auto histo = [width = _width, count = optional_cast<std::size_t>(_bins)](const auto f, const auto l){
                if      (!width && !count) return histogram{f, l};
                else if ( width && !count) return histogram{f, l, *width};
                else if (!width &&  count) return histogram{f, l, *count};
                else throw std::invalid_argument{"Cannot fix bin width and bin count at the same time"};
            }(first, last);
            write_frequencies(histo, _output);
            _update_info(info, subinfo, histo);
        } else if (_kernel == kernels::gaussian) {
            const auto summary = get_stochastic_summary(first, last);
            const auto lo = _lower.value_or(summary.min);
            const auto hi = _upper.value_or(summary.max);
            if (lo > hi) {
                return false;
            }
            const auto scott = summary.stdev() > 0.0
                ? binwidth_scott_normal_reference(summary.count, summary.stdev())
                : 1.0;
            const auto other = _bins.has_value()
                ? std::make_optional(((lo < hi) && (*_bins > 1)) ? (hi - lo) / (*_bins - 1) : 1.0)
                : std::nullopt;
            const auto sigma = _width.value_or(other.value_or(scott)) / 2.0;
            if (sigma <= 0.0) {
                throw std::invalid_argument{"sigma must be positive"};
            }
            const auto filter = gaussian_kernel{first, last, sigma};
            const auto delta = 3.0 * sigma;
            const auto density = _points
                ? make_density(filter, lo - delta, hi + delta, *_points)
                : make_density_adaptive(filter, lo - delta, hi + delta);
            write_density(density, summary, _output);
            const auto entropy = get_differential_entropy_of_pdf(density);
            const auto compx = [](auto&& lhs, auto&& rhs){ return lhs.first  < rhs.first; };
            const auto compy = [](auto&& lhs, auto&& rhs){ return lhs.second < rhs.second; };
            const auto maxx = std::max_element(std::begin(density), std::end(density), compx)->first;
            const auto maxy = std::max_element(std::begin(density), std::end(density), compy)->second;
            _update_info(info, subinfo, summary, sigma, density.size(), entropy, std::make_pair(maxx, maxy));
        } else if (_kernel == kernels::raw) {
            const auto events = std::vector<double>{first, last};
            const auto summary = get_stochastic_summary(events);
            write_events(events, summary, _output);
            _update_info(info, subinfo, summary);
        } else {
            reject_invalid_enumeration(_kernel, "msc::kernels");
        }
        return true;
    }

    inline kernels data_analyzer::get_kernel() const noexcept
    {
        return _kernel;
    }

    inline void data_analyzer::set_kernel(const kernels kern)
    {
        assert(kern != kernels{});
        _kernel = kern;
    }

    inline std::optional<double> data_analyzer::get_lower() const noexcept
    {
        return _lower;
    }

    inline std::optional<double> data_analyzer::get_upper() const noexcept
    {
        return _upper;
    }

    inline void data_analyzer::set_range() noexcept
    {
        _lower = std::nullopt;
        _upper = std::nullopt;
    }

    inline void data_analyzer::set_range(const std::optional<double> lower, const std::optional<double> upper)
    {
        assert(!lower || std::isfinite(*lower));
        assert(!upper || std::isfinite(*upper));
        assert(lower.value_or(DBL_MIN) <= upper.value_or(DBL_MAX));
        _lower = lower;
        _upper = upper;
    }

    inline std::optional<double> data_analyzer::get_width() const noexcept
    {
        return _width;
    }

    inline void data_analyzer::set_width(const std::optional<double> width)
    {
        assert(!width || (std::isfinite(*width) && (*width > 0.0)));
        _width = width;
    }

    inline std::optional<int> data_analyzer::get_bins() const noexcept
    {
        return _bins;
    }

    inline void data_analyzer::set_bins(const std::optional<int> bins)
    {
        assert(!bins || *bins > 0);
        _bins = bins;
    }

    inline std::optional<int> data_analyzer::get_points() const noexcept
    {
        return _points;
    }

    inline void data_analyzer::set_points(const std::optional<int> points)
    {
        assert(!points || *points > 0);
        _points = points;
    }

    inline const output_file& data_analyzer::get_output() const noexcept
    {
        return _output;
    }

    inline void data_analyzer::set_output(output_file dst)
    {
        _output = std::move(dst);
    }

}  // namespace msc
