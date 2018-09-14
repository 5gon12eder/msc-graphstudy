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
#include <cstdlib>
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
#include "random.hxx"

#define PROGRAM_NAME "tree"

namespace /*anonymous*/
{

    template <typename EngineT, typename DegDistT>
    void recurse_on_tree(EngineT& engine, DegDistT& degdist, ogdf::Graph& graph, const ogdf::node node, const int n)
    {
        const auto dbl = [](const auto x)->double{ return x; };
        auto children = std::vector<ogdf::node>(degdist(engine) - degdist.min());
        std::generate(std::begin(children), std::end(children), [&graph](){ return graph.newNode(); });
        std::for_each(std::begin(children), std::end(children), [&graph, node](auto v){ graph.newEdge(node, v); });
        const auto p = std::sqrt(1.0 - std::clamp(dbl(graph.numberOfNodes()) / dbl(n), 0.0, 1.0));
        auto recdist = std::bernoulli_distribution{p};
        for (const auto v : children) {
            if (recdist(engine)) {
                recurse_on_tree(engine, degdist, graph, v, n);
            }
        }
    }

    template <typename EngineT>
    std::unique_ptr<ogdf::Graph> make_tree(EngineT& engine, const int n)
    {
        assert(n > 0);
        while (true) {
            auto metadist = std::uniform_real_distribution{std::min(0.5, 1.0 / n), 0.5};
            auto degdist = std::geometric_distribution{metadist(engine)};
            auto graph = std::make_unique<ogdf::Graph>();
            const auto root = graph->newNode();
            recurse_on_tree(engine, degdist, *graph, root, n);
            if ((graph->numberOfNodes() >= n / 10.0) && (graph->numberOfNodes() <= n * 10.0)) {
                return std::move(graph);
            }
        }
    }

    struct cli_parameters
    {
        msc::output_file output{"-"};
        msc::output_file meta{};
        int nodes{100};
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

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        std::srand(std::uniform_int_distribution<unsigned>{}(rndeng));
        ogdf::setSeed(std::uniform_int_distribution<int>{}(rndeng));
        const auto graph = make_tree(rndeng, this->parameters.nodes);
        msc::store_graph(*graph, this->parameters.output);
        msc::print_meta(get_info(*graph, seed, this->parameters.output), this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generates a random tree.");
    return app(argc, argv);
}
