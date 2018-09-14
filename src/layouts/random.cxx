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

#include <memory>
#include <random>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "enums/distributions.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "random.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "random"

namespace /*anonymous*/
{

    template <typename DistT, typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    make_random_layout_with_distribution(const ogdf::Graph& graph, DistT dist, EngineT& engine)
    {
        auto attrs = std::make_unique<ogdf::GraphAttributes>(graph);
        for (const auto v : graph.nodes) {
            attrs->x(v) = dist(engine);
            attrs->y(v) = dist(engine);
        }
        return attrs;
    }

    template <typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    make_random_layout(const ogdf::Graph& graph, EngineT& engine, const msc::distributions dist)
    {
        switch (dist) {
        case msc::distributions::uniform:
            return make_random_layout_with_distribution(graph, std::uniform_real_distribution<>{}, engine);
        case msc::distributions::normal:
            return make_random_layout_with_distribution(graph, std::normal_distribution<>{}, engine);
        }
        msc::reject_invalid_enumeration(dist, "msc::distributions");
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file meta{};
        msc::distributions distribution{msc::distributions::uniform};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const std::string& seed, const msc::output_file& dst)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["seed"] = seed;
        info["filename"] = msc::make_json(dst.filename());
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        const auto graph = msc::load_graph(this->parameters.input);
        auto attrs = make_random_layout(*graph, rndeng, this->parameters.distribution);
        msc::normalize_layout(*attrs);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes a garbage layout for the given graph by placing nodes at random positions.");
    return app(argc, argv);
}
