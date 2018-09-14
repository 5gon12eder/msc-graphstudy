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

#include <cstdlib>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/basic.h>
#include <ogdf/energybased/DavidsonHarelLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/energybased/GEMLayout.h>
#include <ogdf/energybased/PivotMDS.h>
#include <ogdf/energybased/SpringEmbedderKK.h>
#include <ogdf/energybased/StressMinimization.h>

#include "cli.hxx"
#include "enums/algorithms.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "random.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "force"

namespace /*anonymous*/
{

    const auto layout_features = ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics;

    template <msc::algorithms Algo>
    struct layouter;

    template <>
    struct layouter<msc::algorithms::fmmm> final
    {

        template <typename EngineT>
        void operator()(EngineT& engine, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs) const
        {
            auto layout = ogdf::FMMMLayout{};
            layout.randSeed(std::uniform_int_distribution<int>{}(engine));
            layout.useHighLevelOptions(true);
            layout.qualityVersusSpeed(ogdf::FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);
            if (attrs.has(layout_features)) {
                layout.newInitialPlacement(false);
                assert(&graph == &attrs.constGraph());
            } else {
                layout.newInitialPlacement(true);
                attrs.init(graph, layout_features);
            }
            layout.call(attrs);
        }

    };

    template <>
    struct layouter<msc::algorithms::stress> final
    {

        template <typename EngineT>
        void operator()(EngineT& /*engine*/, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs) const
        {
            if (attrs.has(layout_features)) {
                throw std::invalid_argument{"Stress minimization cannot make use of an initial layout"};
            }
            attrs.init(graph, layout_features);
            attrs.directed() = false;
            auto layout = ogdf::StressMinimization{};
            layout.call(attrs);
        }

    };

    template <>
    struct layouter<msc::algorithms::davidson_harel> final
    {

        template <typename EngineT>
        void operator()(EngineT& /*engine*/, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs) const
        {
            auto layout = ogdf::DavidsonHarelLayout{};
            layout.setPreferredEdgeLength(msc::default_node_distance);
            if (attrs.has(layout_features)) {
                throw std::invalid_argument{"Davidson-Harel layout algorithm cannot make use of an initial layout"};
            }
            attrs.init(graph, layout_features);
            attrs.directed() = false;
            layout.call(attrs);
        }

    };

    template <>
    struct layouter<msc::algorithms::spring_embedder_kk> final
    {

        template <typename EngineT>
        void operator()(EngineT& /*engine*/, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs) const
        {
            auto layout = ogdf::SpringEmbedderKK{};
            layout.setDesLength(msc::default_node_distance);
            if (attrs.has(layout_features)) {
                layout.setUseLayout(true);
            } else {
                layout.setUseLayout(false);
                attrs.init(graph, layout_features);
                attrs.directed() = false;
            }
            layout.call(attrs);
        }

    };

    template <>
    struct layouter<msc::algorithms::pivot_mds> final
    {

        template <typename EngineT>
        void operator()(EngineT& /*engine*/, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs) const
        {
            auto layout = ogdf::PivotMDS{};
            if (attrs.has(layout_features)) {
                throw std::invalid_argument{"Pivot MDS layout algorithm cannot make use of an initial layout"};
            }
            attrs.init(graph, layout_features);
            attrs.directed() = false;
            layout.call(attrs);
        }

    };

    template <typename EngineT>
    void do_layout(EngineT& engine, const ogdf::Graph& graph, ogdf::GraphAttributes& attrs, const msc::algorithms algo)
    {
        // Nobody knows how non-determinism works in the OGDF.  Probably, it doesn't...
        std::srand(std::uniform_int_distribution<unsigned>{}(engine));
        ogdf::setSeed(std::uniform_int_distribution<int>{}(engine));
        switch (algo) {
        case msc::algorithms::fmmm:
            return layouter<msc::algorithms::fmmm>{}(engine, graph, attrs);
        case msc::algorithms::stress:
            return layouter<msc::algorithms::stress>{}(engine, graph, attrs);
        case msc::algorithms::davidson_harel:
            return layouter<msc::algorithms::davidson_harel>{}(engine, graph, attrs);
        case msc::algorithms::spring_embedder_kk:
            return layouter<msc::algorithms::spring_embedder_kk>{}(engine, graph, attrs);
        case msc::algorithms::pivot_mds:
            return layouter<msc::algorithms::pivot_mds>{}(engine, graph, attrs);
        }
        msc::reject_invalid_enumeration(static_cast<int>(algo), "algorithms");
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file meta{};
        msc::algorithms algorithm{};
        bool layout{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs,
                              const msc::algorithms algo,
                              const msc::output_file& dst,
                              const std::string& seed)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["algorithm"] = name(algo);
        info["filename"] = msc::make_json(dst.filename());
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::default_random_engine{};
        const auto seed = msc::seed_random_engine(rndeng);
        auto graph = std::unique_ptr<ogdf::Graph>{};
        auto attrs = std::unique_ptr<ogdf::GraphAttributes>{};
        if (this->parameters.layout) {
            std::tie(graph, attrs) = msc::load_layout(this->parameters.input);
        } else {
            graph = msc::load_graph(this->parameters.input);
            attrs = std::make_unique<ogdf::GraphAttributes>();
        }
        do_layout(rndeng, *graph, *attrs, this->parameters.algorithm);
        msc::normalize_layout(*attrs);
        msc::store_layout(*attrs, this->parameters.output);
        const auto info = get_info(*attrs, this->parameters.algorithm, this->parameters.output, seed);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes a layout for the given graph using the specified fore-directed algorithm.");
    return app(argc, argv);
}
