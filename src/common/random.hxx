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
 * @file random.hxx
 *
 * @brief
 *     Common random utility functions.
 *
 */

#ifndef MSC_RANDOM_HXX
#define MSC_RANDOM_HXX

#include <cstddef>
#include <string>

namespace msc
{

    /**
     * Seeds any random number engine.
     *
     * This process is deterministic if and only if the environment variable `MSC_RANDOM_SEED` is set.  Setting this
     * environment variable to the value returned by this function will cause subsequent calls to this function to have
     * the exact same effect.
     *
     * @param engine
     *     engine to seed
     *
     * @returns
     *     effective random seed
     *
     */
    template <typename EngT>
    std::string seed_random_engine(EngT& engine);

    /**
     * @brief
     *     Deterministically returns a string with the requestd number of random bytes encoded in hex.
     *
     * The string will contain (from left to right) the `bytes` hex encoded values that are obtained by successive calls
     * to `engine` and using the least significant 8 bits from each result.  When `engine` is a default-constructed
     * engine of type `std::mt19937`, the returned string is `5cf6ee792cdf05e1ba2b6325c41a5f10`.
     *
     * @tparam EngineT
     *     pseudo-random number generator type
     *
     * @param engine
     *     pseudo-random number generator
     *
     * @param bytes
     *     number of random bytes (string will have twice as many characters)
     *
     * @returns
     *     pseudo-random hex string
     *
     */
    template <typename EngineT>
    std::string random_hex_string(EngineT& engine, const std::size_t bytes = 16);

}  // namespace msc

#define MSC_INCLUDED_FROM_RANDOM_HXX
#include "random.txx"
#undef MSC_INCLUDED_FROM_RANDOM_HXX

#endif  // !defined(MSC_RANDOM_HXX)
