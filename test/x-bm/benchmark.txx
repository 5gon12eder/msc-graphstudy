// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2016 Moritz Klammler <moritz.klammler@student.kit.edu>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef MSC_INCLUDED_FROM_BENCHMARK_HXX
#error "Never `#include <benchmark.txx>` directly; `#include <benchmark.hxx>` instead."
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

namespace msc::benchmark
{

    namespace benchmark_detail
    {

        template <typename FwdIterT>
        auto mean_stdev_n(const FwdIterT first, const FwdIterT last) {
            const auto n = static_cast<std::size_t>(std::distance(first, last));
            const auto real_n = static_cast<double>(n);
            assert(n >= 3);
            const auto meansumop = [](auto&& accu, auto&& next){
                return accu + next.count();
            };
            const auto mean = std::accumulate(first, last, 0.0, meansumop) / real_n;
            const auto varsumop = [mean](auto&& accu, auto&& next){
                const auto diff = next.count() - mean;
                return accu + diff * diff;
            };
            const auto var = std::accumulate(first, last, 0.0, varsumop) / (real_n - 1.0);
            const auto stdev = std::sqrt(var);
            return result{duration_type{mean}, duration_type{stdev}, n};
        }

        template <typename FwdIterT>
        result do_statistics(const FwdIterT first, const FwdIterT last,
                             const std::size_t warmup, const double quantile) {
            const auto skip = static_cast<std::ptrdiff_t>(warmup);
            auto data = std::vector<duration_type>{first + skip, last};
            const auto real_size = static_cast<double>(data.size());
            const auto real_good = std::round(quantile * real_size);
            const auto good = static_cast<std::size_t>(real_good);
            std::sort(std::begin(data), std::end(data));
            data.resize(good);
            return mean_stdev_n(std::begin(data), std::end(data));
        }

        void print_verbose_progress(std::size_t i, duration_type t);

        void print_constraints(const constraints& c);

    }  // namespace benchmark_detail

    template <typename CallT, typename... ArgTs>
    result run_benchmark(const constraints& c, CallT&& bench, ArgTs&&... args)
    {
        const auto minruns = c.warmup + static_cast<std::size_t>(std::ceil(3.0 / c.quantile));
        auto timings = std::vector<duration_type>{};
        const auto t0 = clock_type::now();
        if (c.verbose) {
            benchmark_detail::print_constraints(c);
        }
        if (c.timeout.count() < 0) {
            throw failure{"Timeout expired before I could do anything useful"};
        }
        while (true) {
            compiler_barrier();
            const auto t1 = clock_type::now();
            compiler_barrier();
            // NB: We are NOT perfectly forwarding the argumetns because we're
            //     invoking the function object more than once so we cannot
            //     give up hold to our arguments.
            bench(args...);
            compiler_barrier();
            const auto t2 = clock_type::now();
            compiler_barrier();
            const auto t = std::chrono::duration_cast<duration_type>(t2 - t1);
            timings.push_back(t);
            if (c.verbose) {
                benchmark_detail::print_verbose_progress(timings.size(), t);
            }
            const auto elapsed = std::chrono::duration_cast<duration_type>(t2 - t0);
            const auto too_long = (c.timeout.count() > 0) && (elapsed >= c.timeout);
            const auto too_often = (c.repetitions > 0) && (timings.size() >= c.repetitions);
            if (timings.size() >= minruns) {
                const auto res = benchmark_detail::do_statistics(
                    std::begin(timings),  std::end(timings), c.warmup, c.quantile
                );
                if (res.stdev.count() == 0.0) {
                    return res;
                } else if (res.stdev.count() / res.mean.count() < c.significance) {
                    return res;
                } else if (too_long || too_often) {
                    return res;
                }
            } else if (too_long) {
                throw failure{"Timeout expired"};
            } else if (too_often) {
                throw failure{"Maximum number of repetitions exceeded"};
            }
        }

    }

}  // namespace msc::benchmark
