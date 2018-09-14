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

#include "angular.hxx"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include <ogdf/basic/GraphAttributes.h>

#include "math_constants.hxx"
#include "point.hxx"
#include "useful.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        void handle_invalid(std::vector<double>& polars, const treatments treatment)
        {
            switch (treatment) {
            case treatments::exception:
                throw std::logic_error{"Indeterminate angle between coinciding vertices"};
            case treatments::ignore:
                return;
            case treatments::replace:
                polars.push_back(NAN);
                return;
            }
            reject_invalid_enumeration(treatment, "msc::treatments");
        }

    }  // namespace /*anonymous*/

    std::vector<double>
    get_all_angles_between_adjacent_incident_edges(const ogdf::GraphAttributes& attrs, const treatments treatment)
    {
            auto angles = std::vector<double>{};
            auto polars = std::vector<double>{};
            for (const auto v : attrs.constGraph().nodes) {
                const auto center = msc::point2d{attrs.x(v), attrs.y(v)};
                for (const auto adj : v->adjEntries) {
                    const auto u = adj->twinNode();
                    if (const auto distance = msc::point2d{attrs.x(u), attrs.y(u)} - center) {
                        polars.push_back(std::atan2(distance.x(), distance.y()));
                    } else {
                        handle_invalid(polars, treatment);
                    }
                }
                if (polars.empty()) {
                    continue;
                }
                std::sort(std::begin(polars), std::end(polars));
                // We could use std::adjacent_difference here if only it would not have the awkward behavior of copying
                // the first item in the list verbatim...
                for (std::size_t i = 1; i < polars.size(); ++i) {
                    angles.push_back(polars[i] - polars[i - 1]);
                }
                angles.push_back(2.0 * M_PI + polars.front() - polars.back());
                polars.clear();
            }
            return angles;
    }

}  // namespace msc
