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

#include "edge_length.hxx"

#include <ogdf/basic/GraphAttributes.h>

#include "point.hxx"

namespace msc
{

    std::vector<double> get_all_edge_lengths(const ogdf::GraphAttributes& attrs)
    {
        auto lengths = std::vector<double>{};
        for (const auto e : attrs.constGraph().edges) {
            const auto p1 = point2d{attrs.x(e->source()), attrs.y(e->source())};
            const auto p2 = point2d{attrs.x(e->target()), attrs.y(e->target())};
            lengths.push_back(distance(p1, p2));
        }
        return lengths;
    }

}  // namespace msc
