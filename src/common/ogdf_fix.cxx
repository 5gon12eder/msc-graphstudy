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

#include "ogdf_fix.hxx"

#include <algorithm>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

namespace msc
{

    msc::point2d get_coords(const ogdf::GraphAttributes& attrs, const ogdf::node v)
    {
        return {attrs.x(v), attrs.y(v)};
    }

    std::pair<msc::point2d, msc::point2d> get_bounding_box(const ogdf::GraphAttributes& attrs) noexcept
    {
        auto sw = make_invalid_point<double, 2>();
        auto ne = make_invalid_point<double, 2>();
        if (attrs.constGraph().numberOfNodes() > 0) {
            sw = get_coords(attrs, attrs.constGraph().firstNode());
            ne = get_coords(attrs, attrs.constGraph().lastNode());
        }
        for (const auto v : attrs.constGraph().nodes) {
            const auto x = attrs.x(v);
            const auto y = attrs.y(v);
            sw.x() = std::min(sw.x(), x);
            sw.y() = std::min(sw.y(), y);
            ne.x() = std::max(ne.x(), x);
            ne.y() = std::max(ne.y(), y);
        }
        return {sw, ne};
    }

    msc::point2d get_bounding_box_size(const ogdf::GraphAttributes& attrs) noexcept
    {
        const auto [sw, ne] = get_bounding_box(attrs);
        return ne - sw;
    }

}  // namespace msc
