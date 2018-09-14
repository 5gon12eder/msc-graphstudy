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
 * @file angular.hxx
 *
 * @brief
 *     Utilities for computing the `ANGULAR` property.
 *
 */

#ifndef MSC_ANGULAR_HXX
#define MSC_ANGULAR_HXX

#include <vector>

#include "enums/treatments.hxx"
#include "ogdf_fwd.hxx"

namespace msc
{

    /**
     * @brief
     *     Computes all angles between adjacent incident edges.
     *
     * For vertices with degree 1 an angle of 2 &pi; will be reported.
     *
     * @param attrs
     *     layout to operate on
     *
     * @param treatment
     *     strategy for treating non-finite angles
     *
     * @returns
     *     angles in unspecified order
     *
     */
    std::vector<double>
    get_all_angles_between_adjacent_incident_edges(const ogdf::GraphAttributes& attrs,
                                                   treatments treatment = treatments::exception);

}  // namespace msc

#endif  // !defined(MSC_ANGULAR_HXX)
