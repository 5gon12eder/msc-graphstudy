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

#include <cmath>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

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
#include "random.hxx"

#define PROGRAM_NAME "grid"

namespace /*anonymous*/
{

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_grid(const int width, const int height, const int torus)
    {
        if (torus > 2) {
            throw std::invalid_argument{"Sorry, N-torii with N > 2 are not supported"};
        }
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        auto row = std::vector<ogdf::node>(width, nullptr);
        auto top = std::vector<ogdf::node>{};
        for (auto i = 0; i < height; ++i) {
            for (auto j = 0; j < width; ++j) {
                const auto v = graph->newNode();
                attrs->x(v) = j;
                attrs->y(v) = i;
                if (const auto u = row[j]) {
                    graph->newEdge(u, v);
                }
                row[j] = v;
            }
            for (auto j = 1; j < width; ++j) {
                graph->newEdge(row[j - 1], row[j]);
            }
            if ((torus >= 1) && (width > 1)) {
                graph->newEdge(row.back(), row.front());
            }
            if ((torus >= 2) && (i == 0)) {
                top = row;
            }
        }
        if ((torus >= 2) && (height > 1)) {
            for (auto j = 0; j < width; ++j) {
                graph->newEdge(row[j], top[j]);
            }
        }
        return {std::move(graph), std::move(attrs)};
    }

    struct cli_parameters
    {
        msc::output_file output{"-"};
        msc::output_file meta{};
        int nodes{100};
        int torus{0};
    };

    struct application
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::Graph& graph, const std::string& seed, const msc::output_file& dst)
    {
        auto info = msc::json_object{};
        info["graph"] = msc::graph_fingerprint(graph);
        info["nodes"] = msc::json_diff{graph.numberOfNodes()};
        info["edges"] = msc::json_diff{graph.numberOfEdges()};
        info["producer"] = PROGRAM_NAME;
        info["seed"] = seed;
        info["filename"] = msc::make_json(dst.filename());
        info["native"] = msc::json_bool{false};
        return info;
    }

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const std::string& seed, const msc::output_file& dst)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = get_info(attrs.constGraph(), seed, dst);
        info["native"] = msc::json_bool{true};
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        const auto maxdim = static_cast<int>(std::round(2 * std::sqrt(this->parameters.nodes)));
        auto rnddst = std::uniform_int_distribution{1, maxdim};
        const auto n = rnddst(rndeng);
        const auto m = rnddst(rndeng);
        auto [graph, attrs] = make_grid(n, m, this->parameters.torus);
        if (this->parameters.torus == 0) {
            msc::normalize_layout(*attrs);
            msc::store_layout(*attrs, this->parameters.output);
            msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
        } else {
            msc::store_graph(*graph, this->parameters.output);
            msc::print_meta(get_info(*graph, seed, this->parameters.output), this->parameters.meta);
        }
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generates a regular grid.");
    return app(argc, argv);
}
