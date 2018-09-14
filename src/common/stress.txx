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

#ifndef MSC_INCLUDED_FROM_STRESS_HXX
#  error "Never `#include <stress.txx>` directly, `#include <stress.hxx>` instead"
#endif

#include "point.hxx"
#include "useful.hxx"

namespace msc
{
    inline double node_stress::operator()(const ogdf::node v1, const ogdf::node v2) const noexcept
    {
        const auto p1 = point2d{_attrs->x(v1), _attrs->y(v1)};
        const auto p2 = point2d{_attrs->x(v2), _attrs->y(v2)};
        const auto dist = distance(p1, p2);
        const auto spl = (*_matrix)[v1][v2];
        return square((dist - _nodesep * spl) / spl);
    }

}  // namespace msc
