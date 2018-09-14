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

#ifndef MSC_INCLUDED_FROM_SLIDING_HXX
#  error "Never `#include <sliding.txx>` directly, `#include <sliding.hxx>` instead"
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

#include "numeric.hxx"

namespace msc
{

    template <typename FwdIterT, typename ValueT, typename SfinaeT>
    gaussian_kernel<FwdIterT, ValueT, SfinaeT>::gaussian_kernel(const FwdIterT first,
                                                                const FwdIterT last,
                                                                const ValueT sigma) :
        _first{first}, _last{last}, _sigma{sigma}
    {
        assert(sigma >= 0.0);
    }

    template <typename FwdIterT, typename ValueT, typename SfinaeT>
    ValueT gaussian_kernel<FwdIterT, ValueT, SfinaeT>::operator()(const ValueT x) const
    {
        const auto kernel = gaussian{x, _sigma};
        auto accu = ValueT{};
        for (auto it = _first; it != _last; ++it) {
            accu += kernel(*it);
        }
        return accu;
    }

    template <typename KernelT>
    std::vector<std::pair<double, double>>
    make_density(const KernelT& kernel,
                 const double minval,
                 const double maxval,
                 const int points,
                 const bool normalize
        )
    {
        assert(minval <= maxval);
        assert(points >= 3);
        const auto steps = points - 1;
        auto density = std::vector<std::pair<double, double>>{};
        density.reserve(points);
        for (auto i = 0; i <= steps; ++i) {
            const auto x = minval + i * (maxval - minval) / steps;
            const auto y = kernel(x);
            density.emplace_back(x, y);
        }
        if (normalize) {
            const auto area = integrate_trapezoidal(std::begin(density), std::end(density));
            assert(area >= 0.0);
            for (auto& pair : density) {
                pair.second /= area;
            }
        }
        return density;
    }

    namespace detail::sliding
    {

        template <typename KernelT>
        class adaptive_density_helper
        {
        public:

            adaptive_density_helper(KernelT kernel, const double minval, const double maxval)
                : _kernel{std::move(kernel)}
            {
                const auto initsteps = 17;
                const auto maxdepth = 10;
                auto whatever = 0.0;
                for (auto i = 0; i <= initsteps; ++i) {
                    const auto x = minval + i * (maxval - minval) / initsteps;
                    const auto y = _kernel(x);
                    _density.emplace_back(x, y);
                    whatever += std::abs(y);
                }
                whatever /= (initsteps + 1);
                if (whatever > 0.0) {
                    _abstol = 5.0E-2 * whatever;
                    for (auto i = 0; i < initsteps; ++i) {
                        _recurse(_density[i], _density[i + 1], maxdepth);
                    }
                    const auto paircmp = [](const auto& lhs, const auto& rhs){ return lhs.first < rhs.first; };
                    std::sort(std::begin(_density), std::end(_density), paircmp);
                }
            }

            std::vector<std::pair<double, double>> operator()() && noexcept
            {
                return std::move(_density);
            }

            void _recurse(const std::pair<double, double> lo, const std::pair<double, double> hi, const int levels)
            {
                constexpr auto steps = 2;
                std::array<double, steps> ts;
                std::array<double, steps> xs;
                std::array<double, steps> ys;
                for (auto i = 0; i < steps; ++i) {
                    ts[i] = _rnddst(_rndeng);
                    xs[i] = lo.first + ts[i] * (hi.first - lo.first);
                    ys[i] = _kernel(xs[i]);
                    _density.emplace_back(xs[i], ys[i]);
                }
                if (levels > 0) {
                    auto done = true;
                    for (auto i = 0; i < steps; ++i) {
                        const auto mi = lo.second + ts[i] * (hi.second - lo.second);
                        if (std::abs(mi - ys[i]) > _abstol) {
                            done = false;
                            break;
                        }
                    }
                    if (!done) {
                        for (auto i = 1; i <= steps; ++i) {
                            const auto loi = (i == 1)     ? lo : std::make_pair(xs[i - 2], ys[i - 2]);
                            const auto hii = (i == steps) ? hi : std::make_pair(xs[i - 1], ys[i - 1]);
                            _recurse(loi, hii, levels - 1);
                        }
                    }
                }
            }

        private:

            KernelT _kernel{};
            std::vector<std::pair<double, double>> _density{};
            double _abstol{};
            std::minstd_rand _rndeng{};
            std::uniform_real_distribution<double> _rnddst{0.0, 1.0};

        };

    }  // namespace detail::sliding

    template <typename KernelT>
    std::vector<std::pair<double, double>>
    make_density_adaptive(const KernelT& kernel, const double minval, const double maxval, const bool normalize)
    {
        assert(minval <= maxval);
        auto density = detail::sliding::adaptive_density_helper{kernel, minval, maxval}();
        if (normalize) {
            const auto area = integrate_trapezoidal(std::begin(density), std::end(density));
            assert(area >= 0.0);
            for (auto& pair : density) {
                pair.second /= area;
            }
        }
        return density;
    }

}  // namespace msc
