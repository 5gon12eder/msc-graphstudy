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

#include <climits>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/energybased/FMMMLayout.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "random.hxx"

#define PROGRAM_NAME "phantom"

namespace /*anonymous*/
{

    const auto dumpenvvar = "MSC_DUMP_PHANTOM";

    template <typename EngineT>
    std::unique_ptr<ogdf::Graph>
    make_random_graph(EngineT& engine, const int n, const int m)
    {
        auto seeddist = std::uniform_int_distribution<int>{INT_MIN, INT_MAX};
        ogdf::setSeed(seeddist(engine));
        assert(n >= 0);
        assert((0 <= m) && (m <= n * n));
        auto graph = std::make_unique<ogdf::Graph>();
        ogdf::randomSimpleGraph(*graph, n, m);
        return graph;
    }

    template <typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    make_phantom_layout(EngineT& engine, const ogdf::Graph& graph)
    {
        const auto _graph = make_random_graph(engine, graph.numberOfNodes(), graph.numberOfEdges());
        auto _attrs = std::make_unique<ogdf::GraphAttributes>(*_graph);
        auto layout = ogdf::FMMMLayout{};
        layout.randSeed(std::uniform_int_distribution<int>{}(engine));
        layout.useHighLevelOptions(true);
        layout.newInitialPlacement(true);
        layout.qualityVersusSpeed(ogdf::FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);
        layout.call(*_attrs);
        msc::normalize_layout(*_attrs);
        if (const auto phantomfile = std::getenv(dumpenvvar)) {
            msc::store_layout(*_attrs, msc::output_file{phantomfile});
        }
        auto attrs = std::make_unique<ogdf::GraphAttributes>(graph);
        auto v = graph.firstNode();
        for (const auto u : _graph->nodes) {
            assert(v != nullptr);
            attrs->x(v) = _attrs->x(u);
            attrs->y(v) = _attrs->y(u);
            v = v->succ();
        }
        assert(v == nullptr);
        msc::normalize_layout(*attrs);
        return attrs;
    }

    struct cli_parameters final
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
        auto attrs = make_phantom_layout(rndeng, *graph);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.environ[dumpenvvar] = "store the phantom layout in a file";
    app.help.push_back(
        "Computes a garbage layout for the given graph by layouting it in a way that matches the force-directed layout"
        " of another random graph with the same number of nodes and edges."
    );
    return app(argc, argv);
}
