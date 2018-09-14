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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "stress.hxx"

#define PROGRAM_NAME "stress"

namespace /*anonymous*/
{

    struct cli_parameters : msc::cli_parameters_metric
    {
        msc::stress_modi stress_modus{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const double stress)
    {
        auto info = msc::json_object{};
        info["stress"] = msc::json_real{stress};
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object get_info(const msc::parabola_result& result, const std::string_view xvarname)
    {
        auto info = msc::json_object{};
        info["stress"] = msc::json_real{result.y0};
        info[xvarname] = msc::json_real{result.x0};
        info["polynomial"] = msc::json_array{
            msc::json_real{result.a},
            msc::json_real{result.b},
            msc::json_real{result.c},
        };
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        const auto [graph, attrs] = msc::load_layout(this->parameters.input);
        switch (this->parameters.stress_modus) {
        case msc::stress_modi::fixed:
            return msc::print_meta(get_info(msc::compute_stress(*attrs)), this->parameters.meta);
        case msc::stress_modi::fit_nodesep:
            return msc::print_meta(get_info(msc::compute_stress_fit_nodesep(*attrs), "nodesep"), this->parameters.meta);
        case msc::stress_modi::fit_scale:
            return msc::print_meta(get_info(msc::compute_stress_fit_scale(*attrs), "scale"), this->parameters.meta);
        }
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes the stress function for a normalized layout.");
    return app(argc, argv);
}
