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

#ifndef MSC_INCLUDED_FROM_RANDOM_HXX
#  error "Never `#include <random.txx>` directly; `#include <random.hxx>` instead"
#endif

#include <random>

namespace msc
{

    namespace detail::random
    {

        std::string get_seed_string();

    }  // namespace detail::random

    template <typename EngT>
    std::string seed_random_engine(EngT& engine)
    {
        const auto seed = detail::random::get_seed_string();
        auto seedseq = std::seed_seq(seed.begin(), seed.end());
        engine.seed(seedseq);
        return seed;
    }

    template <typename EngineT>
    std::string random_hex_string(EngineT& engine, const std::size_t bytes)
    {
        // NB: We do not use a distribution here because unlike for engines, the ISO C++ standard does not specify the
        // sequence of numbers they produce.
        static const char hexdigits[] = "0123456789abcdef";
        auto result = std::string{};
        result.reserve(2 * bytes);
        for (auto i = std::size_t{}; i < bytes; ++i) {
            const auto byte = engine() & 0xffU;
            const auto lo = ((byte >> 0) & 0xfU);
            const auto hi = ((byte >> 4) & 0xfU);
            result.push_back(hexdigits[hi]);
            result.push_back(hexdigits[lo]);
        }
        return result;
    }

}  // namespace msc
