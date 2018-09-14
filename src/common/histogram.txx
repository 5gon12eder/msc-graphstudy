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

#ifndef MSC_INCLUDED_FROM_HISTOGRAM_HXX
#  error "Never `#include <histogram.txx>` directly; `#include <histogram.hxx>` instead"
#endif

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace msc
{

    inline histogram::histogram() noexcept
    {
    }

    template <typename FwdIterT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const FwdIterT first, const FwdIterT last)
        : _binning{binnings::scott_normal_reference}
    {
        static_assert(std::is_convertible_v<typename std::iterator_traits<FwdIterT>::value_type, ValueT>);
        assert(first != last);
        _summary = get_stochastic_summary(first, last);
        if (!(_summary.count >= 3)) {
            throw std::invalid_argument{"At least three events are needed for a meaningful histogram"};
        }
        const auto range = (_summary.max - _summary.min);
        const auto stdev = _summary.stdev();
        _binwidth = ((range > FLT_MIN) && (stdev > FLT_MIN))
            ? binwidth_scott_normal_reference(_summary.count, stdev)
            : 1.0;
        const auto bins = (range > 0.0) ? std::round(1.0 + range / _binwidth) : 1.0;
        _frequencies.resize(static_cast<std::size_t>(bins));
        _start = (_summary.min + _summary.max - _binwidth * bins) / 2.0;
        _entropy = _init_histo(first, last);
    }

    template <typename ContainerT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const ContainerT& events)
        : histogram{std::begin(events), std::end(events)}
    {
    }

    template <typename FwdIterT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const FwdIterT first, const FwdIterT last, const std::size_t bincount)
        : _binning{binnings::fixed_count}
    {
        assert(first != last);
        assert(bincount > 0);
        _summary = get_stochastic_summary(first, last);
        if (!(_summary.count >= 3)) {
            throw std::invalid_argument{"At least three events are needed for a meaningful histogram"};
        }
        const auto range = (_summary.max - _summary.min);
        _binwidth = ((range > 0.0) && (bincount > 1)) ? range / (bincount - 1) : 1.0;
        _frequencies.resize(bincount);
        _start = (_summary.min + _summary.max - _binwidth * bincount) / 2.0;
        _entropy = _init_histo(first, last);
    }

    template <typename ContainerT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const ContainerT& events, const std::size_t bincount)
        : histogram{std::begin(events), std::end(events), bincount}
    {
    }

    template <typename FwdIterT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const FwdIterT first, const FwdIterT last, const double binwidth)
        : _binning{binnings::fixed_width}
    {
        assert(first != last);
        assert(binwidth > 0.0);
        _binwidth = binwidth;
        _summary = get_stochastic_summary(first, last);
        if (!(_summary.count >= 3)) {
            throw std::invalid_argument{"At least three events are needed for a meaningful histogram"};
        }
        const auto range = (_summary.max - _summary.min);
        const auto bins = (range > 0.0) ? std::round(1.0 + range / binwidth) : 1.0;
        _frequencies.resize(static_cast<std::size_t>(bins));
        _start = (_summary.min + _summary.max - _binwidth * bins) / 2.0;
        _entropy = _init_histo(first, last);
    }

    template <typename ContainerT, typename ValueT, typename /*SFINAE*/>
    histogram::histogram(const ContainerT& events, const double binwidth)
        : histogram{std::begin(events), std::end(events), binwidth}
    {
    }

    inline histogram::histogram(histogram&& other) noexcept : histogram{}
    {
        swap(*this, other);
    }

    inline histogram& histogram::operator=(histogram&& other) noexcept
    {
        histogram temp{};
        swap(*this, temp);
        swap(*this, other);
        return *this;
    }

    inline double histogram::center(const std::size_t idx) const
    {
        assert(idx < _frequencies.size());
        return _start + (idx + 0.5) * _binwidth;
    }

    inline double histogram::frequency(const std::size_t idx) const
    {
        assert(idx < _frequencies.size());
        return _frequencies[idx];
    }

    template <typename FwdIterT>
    double histogram::_init_histo(const FwdIterT first, const FwdIterT last)
    {
        assert(!_frequencies.empty());
        assert(_binwidth > 0.0);
        assert(std::all_of(std::cbegin(_frequencies), std::cend(_frequencies), [](double x){ return (x == 0.0); }));
        const auto bins = static_cast<double>(_frequencies.size());
        assert(static_cast<std::size_t>(bins) == _frequencies.size());
        for (auto it = first; it != last; ++it) {
            const auto event = *it;
            const auto bin = std::floor((event - _start) / _binwidth);
            if ((bin >= 0.0) && (bin < bins)) {
                const auto idx = static_cast<std::size_t>(bin);
                _frequencies[idx] += 1.0;
            }
        }
        const auto total = std::accumulate(std::cbegin(_frequencies), std::cend(_frequencies), 0.0);
        assert(std::isfinite(total) && (total >= 0.0) && (total <= std::distance(first, last)));
        if (total > 0.0) {
            const auto normalize = [total](const double x){ return x / total; };
            std::transform(std::cbegin(_frequencies), std::cend(_frequencies), std::begin(_frequencies), normalize);
            return ::msc::entropy(_frequencies);
        }
        return 0.0;
    }

}  // namespace msc
