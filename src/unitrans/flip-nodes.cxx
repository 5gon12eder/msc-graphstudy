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
#include "random.hxx"

#define PROGRAM_NAME "flip-nodes"

namespace /*anonymous*/
{

    // This function exists because ogdf::Graph::nodes is not indexable, does not provide an STL-compliant iterator and
    // ogdf::Graph::allNodes<ContainerT> helpfully uses ContainerT::pushBack which std::vector<ogdf::node> obviously
    // does not provide...
    std::vector<ogdf::node> get_nodes(const ogdf::Graph& graph)
    {
        auto nodes = std::vector<ogdf::node>{};
        nodes.reserve(graph.numberOfNodes());
        for (const auto v :graph.nodes) {
            nodes.push_back(v);
        }
        return nodes;
    }

    template <typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    worsen(EngineT /*by-value*/ engine, const ogdf::GraphAttributes& attrs, const double rate)
    {
        const auto nodes = get_nodes(attrs.constGraph());
        auto flipdist = std::uniform_real_distribution{};
        auto nodedist = nodes.empty()
            ? std::uniform_int_distribution<std::size_t>{}
            : std::uniform_int_distribution<std::size_t>{0, nodes.size() - 1};
        auto worse = std::make_unique<ogdf::GraphAttributes>(attrs.constGraph());
        for (const auto v : nodes) {
            const auto other = nodes[nodedist(engine)];
            const auto u = (flipdist(engine) < rate) ? other : v;
            worse->x(v) = attrs.x(u);
            worse->y(v) = attrs.y(u);
        }
        msc::normalize_layout(*worse);
        return worse;
    }

    struct application final
    {
        msc::cli_parameters_worsening parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const std::string& seed)
    {
        auto info = msc::json_object{};
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object get_subinfo(const ogdf::GraphAttributes& attrs, const double rate, const std::string& filename)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["filename"] = filename;
        info["layout"] = msc::layout_fingerprint(attrs);
        info["rate"] = msc::json_real{rate};
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        auto [graph, attrs] = msc::load_layout(this->parameters.input);
        auto info = get_info(seed);
        auto data = msc::json_array{};
        for (const auto rate : this->parameters.rate) {
            const auto dest = this->parameters.expand_filename(rate);
            const auto worse = worsen(rndeng, *attrs, rate);
            msc::store_layout(*worse, dest);
            data.push_back(get_subinfo(*worse, rate, dest.filename()));
        }
        info["data"] = std::move(data);
        msc::print_meta(info, this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Worsens a given layout by randomly flipping pairs of nodes.");
    return app(argc, argv);
}
