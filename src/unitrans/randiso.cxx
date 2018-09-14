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
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "random.hxx"

#define PROGRAM_NAME "randiso"

namespace /*anonymous*/
{

    template <typename EngineT>
    void randomize(EngineT& engine, ogdf::GraphAttributes& attrs)
    {
        auto coords = std::vector<msc::point2d>{};
        coords.reserve(attrs.constGraph().numberOfNodes());
        for (const auto v : attrs.constGraph().nodes) {
            coords.emplace_back(attrs.x(v), attrs.y(v));
        }
        std::shuffle(std::begin(coords), std::end(coords), engine);
        for (const auto v : attrs.constGraph().nodes) {
            const auto [x, y] = coords[v->index()];
            attrs.x(v) = x;
            attrs.y(v) = y;
        }
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file meta{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const std::string& seed)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        auto [graph, attrs] = msc::load_layout(this->parameters.input);
        randomize(rndeng, *attrs);
        msc::normalize_layout(*attrs);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed), this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back(
        "Worsens a given layout by re-assigning all nodes to a random permutation (isomorphic graph)."
    );
    return app(argc, argv);
}
