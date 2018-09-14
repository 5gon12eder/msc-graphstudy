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

#include <algorithm>
#include <cmath>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "angular.hxx"
#include "cli.hxx"
#include "edge_crossing.hxx"
#include "edge_length.hxx"
#include "io.hxx"
#include "json.hxx"
#include "math_constants.hxx"
#include "meta.hxx"
#include "stochastic.hxx"

#define PROGRAM_NAME "huang"

namespace /*anonymous*/
{

    struct cli_parameters : msc::cli_parameters_metric
    {
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const std::size_t cross_count,
                              const double cross_resolution,
                              const double angular_resolution,
                              const double edge_length_stdev)
    {
        auto info = msc::json_object{};
        info["cross-count"] = msc::json_size{cross_count};
        info["cross-resolution"] = msc::json_real{cross_resolution};
        info["angular-resolution"] = msc::json_real{angular_resolution};
        info["edge-length-stdev"] = msc::json_real{edge_length_stdev};
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        const auto [graph, attrs] = msc::load_layout(this->parameters.input);
        const auto crossings = msc::find_edge_crossings(*attrs);
        const auto cross_count = crossings.size();
        const auto cross_resolution = [&attrs, &crossings]()->double{
            auto temp = 2.0 * M_PI;
            for (const auto& item : crossings) {
                const auto [e1, e2] = std::pair(std::get<1>(item), std::get<2>(item));
                const auto angle = msc::get_crossing_angle(*attrs, e1, e2);
                if (std::isfinite(angle)) temp = std::min(temp, angle);
            }
            return temp;
        }();
        const auto angular = get_all_angles_between_adjacent_incident_edges(*attrs, msc::treatments::ignore);
        const auto angular_resolution = *std::min_element(std::begin(angular), std::end(angular));
        const auto edge_lengths = msc::get_all_edge_lengths(*attrs);
        const auto edge_length_stdev = msc::mean_stdev(edge_lengths).second;
        msc::print_meta(get_info(cross_count, cross_resolution, angular_resolution, edge_length_stdev),
                        this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes the inputs of the combined metric by Huang et alii for a normalized layout.");
    return app(argc, argv);
}
