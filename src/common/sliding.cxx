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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sliding.hxx"

#include <cmath>
#include <utility>

#include <boost/iterator/transform_iterator.hpp>

#include "numeric.hxx"

namespace msc
{

    double get_differential_entropy_of_pdf(const std::vector<std::pair<double, double>>& density)
    {
        struct boost_wants_no_lambda final
        {
            std::pair<double, double> operator()(const std::pair<double, double> xp) const noexcept
            {
                const auto [x, p] = xp;
                const auto plog2p = (p > 0.0) ? -(p * std::log2(p)) : 0.0;
                return std::make_pair(x, plog2p);
            }
        };
        const auto first = boost::make_transform_iterator(std::begin(density), boost_wants_no_lambda{});
        const auto last = boost::make_transform_iterator(std::end(density), boost_wants_no_lambda{});
        return integrate_trapezoidal(first, last);
    }

}  // namespace msc
