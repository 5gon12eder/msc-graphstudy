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
#  include <config.h>
#endif

#include "random.hxx"

#include <cstdlib>
#include <random>
#include <utility>

namespace msc
{

    namespace detail::random
    {

        std::string get_seed_string()
        {
            if (const auto envval = std::getenv("MSC_RANDOM_SEED")) {
                return envval;
            } else {
                auto rnddev = std::random_device{};
                return random_hex_string(rnddev, 24);
            }
        }

    }  // namespace detail::random

}  // namespace msc
