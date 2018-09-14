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

#include "edge_crossing.hxx"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <utility>

#include <ogdf/basic/GraphAttributes.h>

#include "ogdf_fix.hxx"

namespace msc
{

    auto find_edge_crossings(const ogdf::GraphAttributes& attrs)
        -> std::vector<std::tuple<point2d, ogdf::edge, ogdf::edge>>
    {
        auto edges = std::vector<ogdf::edge>{};
        edges.reserve(attrs.constGraph().numberOfEdges());
        for (const auto e : attrs.constGraph().edges) {
            edges.push_back(e);
        }
        const auto lefterx = [&attrs](const auto e1, const auto e2)->bool{
            const auto x1 = std::min(attrs.x(e1->source()), attrs.x(e1->target()));
            const auto x2 = std::min(attrs.x(e2->source()), attrs.x(e2->target()));
            return x1 < x2;
        };
        std::sort(std::begin(edges), std::end(edges), lefterx);
        auto crossings = std::vector<std::tuple<point2d, ogdf::edge, ogdf::edge>>{};
        for (auto it1 = std::begin(edges); it1 != std::end(edges); ++it1) {
            for (auto it2 = std::next(it1); it2 != std::end(edges); ++it2) {
                const auto [e1, e2] = std::make_pair(*it1, *it2);
                if (e1->source() == e2->source()) continue;
                if (e1->source() == e2->target()) continue;
                if (e1->target() == e2->source()) continue;
                if (e1->target() == e2->target()) continue;
                auto l1 = std::make_pair(get_coords(attrs, e1->source()), get_coords(attrs, e1->target()));
                auto l2 = std::make_pair(get_coords(attrs, e2->source()), get_coords(attrs, e2->target()));
                if (l1.first.x() > l1.second.x()) { std::swap(l1.first, l1.second); }
                if (l2.first.x() > l2.second.x()) { std::swap(l2.first, l2.second); }
                if (l2.first.x() > l1.second.x()) { break; }
                auto intersection = check_intersect(l1, l2);
                if (intersection.has_value()) {
                    crossings.emplace_back(*intersection, *it1, *it2);
                }
            }
        }
        return crossings;
    }

    auto get_crossing_angle(const ogdf::GraphAttributes& attrs, const ogdf::edge e1, const ogdf::edge e2) -> double
    {
        const auto p1 = get_coords(attrs, e1->target()) - get_coords(attrs, e1->source());
        const auto p2 = get_coords(attrs, e2->target()) - get_coords(attrs, e2->source());
        if (!p1 || !p2) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        const auto p1n = normalized(p1);
        const auto p2n = normalized(p2);
        const auto dp = dot(p1n, p2n);
        return std::acos(dp);
    }

}  // namespace msc
