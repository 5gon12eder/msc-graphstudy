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

#ifndef MSC_INCLUDED_FROM_REGRESSION_HXX
#  error "Never `#include <regression.txx>` directly, `#include <regression.hxx>` instead"
#endif

#include <limits>

#include "stochastic.hxx"  // for square

namespace msc
{

    template <typename FwdIterT, typename PairT, typename T1, typename T2, typename T, typename /*SFINAE*/>
    std::array<T, 2> linear_regression(const FwdIterT first, const FwdIterT last)
    {
        constexpr auto zero = T{0};
        const auto n = std::distance(first, last);
        if (n == 0) {
            const auto nan = std::numeric_limits<T>::quiet_NaN();
            return {nan, nan};
        }
        const auto xmean = std::accumulate(
            first, last, zero,[](const T accu, const PairT& next){ return accu + std::get<0>(next); }
        ) / n;
        const auto ymean = std::accumulate(
            first, last, zero, [](const T accu, const PairT& next){ return accu + std::get<1>(next); }
        ) / n;
        const auto numer = std::accumulate(
            first, last, zero, [xmean, ymean](const T accu, const PairT& next){
                return accu + (std::get<0>(next) - xmean) * (std::get<1>(next) - ymean);
            });
        const auto denum = std::accumulate(
            first, last, zero, [xmean](const T accu, const PairT& next){
                return accu + square(std::get<0>(next) - xmean);
            });
        const auto k = (denum != zero) ? numer / denum : zero;
        const auto d = ymean - k * xmean;
        return {d, k};
    }

}  // namespace msc
