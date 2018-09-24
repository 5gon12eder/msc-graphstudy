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
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/numeric/ublas/matrix.hpp>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "enums/fileformats.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "import"

namespace /*anonymous*/
{

    std::pair<ogdf::node, ogdf::node> order_nodes(const ogdf::node v1, const ogdf::node v2) noexcept
    {
        return std::minmax(v1, v2);
    }

    template <typename T>
    void fill_with_zeros(boost::numeric::ublas::matrix<T>& matrix) noexcept
    {
        matrix = boost::numeric::ublas::zero_matrix<T>(matrix.size1(), matrix.size2());
    }

    void check_graph(const ogdf::Graph& graph)
    {
        const auto n = graph.numberOfNodes();
        auto adjmat = boost::numeric::ublas::matrix<bool>(n, n);
        fill_with_zeros(adjmat);
        for (const auto e : graph.edges) {
            const auto [v1, v2] = order_nodes(e->source(), e->target());
            if (v1 == v2) {
                throw std::invalid_argument{"Graph contains loops"};
            }
            if (std::exchange(adjmat(v1->index(), v2->index()), true)) {
                throw std::invalid_argument{"Graph contains multiple edges"};
            }
        }
    }

    std::unique_ptr<ogdf::Graph> simplify_graph(const ogdf::Graph& graph)
    {
        auto simple = std::make_unique<ogdf::Graph>();
        auto vertices = std::vector<ogdf::node>(graph.numberOfNodes());
        std::generate(std::begin(vertices), std::end(vertices), [&simple](){ return simple->newNode(); });
        for (const auto e : graph.edges) {
            const auto [v1, v2] = order_nodes(e->source(), e->target());
            if (v1 != v2) {
                const auto u1 = vertices.at(v1->index());
                const auto u2 = vertices.at(v2->index());
                if (simple->searchEdge(u1, u2) == nullptr) {
                    simple->newEdge(u1, u2);
                }
            }
        }
        return simple;
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file output_layout{};
        msc::output_file meta{};
        msc::fileformats format{};
        std::optional<bool> layout{};
        bool simplify{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::Graph& graph, const msc::output_file& dst)
    {
        auto info = msc::json_object{};
        info["graph"] = msc::graph_fingerprint(graph);
        info["nodes"] = msc::json_diff{graph.numberOfNodes()};
        info["edges"] = msc::json_diff{graph.numberOfEdges()};
        info["filename"] = msc::make_json(dst.filename());
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object get_info(const ogdf::GraphAttributes& attrs,
                              const msc::output_file& output,
                              const msc::output_file& output_layout)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = get_info(attrs.constGraph(), output);
        if (output_layout.terminal() != msc::terminals::null) {
            info["filename-layout"] = msc::make_json(output_layout.filename());
        }
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void store_graph_and_layout(const ogdf::GraphAttributes& attrs,
                                const msc::output_file output,
                                const msc::output_file output_layout)
    {
        if (output_layout.terminal() == msc::terminals::null) {
            msc::store_layout(attrs, output);
        } else {
            msc::store_graph(attrs.constGraph(), output);
            msc::store_layout(attrs, output_layout);
        }
    }

    void application::operator()() const
    {
        if (!this->parameters.layout.has_value() && !this->parameters.simplify) {
            const auto [graph, attrs] = msc::import_layout_or_graph(this->parameters.input, this->parameters.format);
            check_graph(*graph);
            if (attrs) {
                msc::normalize_layout(*attrs);
                store_graph_and_layout(*attrs, this->parameters.output, this->parameters.output_layout);
                const auto info = get_info(*attrs, this->parameters.output, this->parameters.output_layout);
                msc::print_meta(info, this->parameters.meta);
            } else {
                msc::store_graph(*graph, this->parameters.output);
                msc::print_meta(get_info(*graph, this->parameters.output), this->parameters.meta);
            }
        } else if (this->parameters.layout.value_or(false) == true) {
            if (this->parameters.simplify) {
                throw std::invalid_argument{"Only graphs with no layout can be simplified"};
            }
            const auto [graph, attrs] = msc::import_layout(this->parameters.input, this->parameters.format);
            check_graph(*graph);
            msc::normalize_layout(*attrs);
            store_graph_and_layout(*attrs, this->parameters.output, this->parameters.output_layout);
            const auto info = get_info(*attrs, this->parameters.output, this->parameters.output_layout);
            msc::print_meta(info, this->parameters.meta);
        } else if (this->parameters.layout.value_or(false) == false) {
            auto graph = msc::import_graph(this->parameters.input, this->parameters.format);
            if (this->parameters.simplify) {
                graph = simplify_graph(*graph);
            } else {
                check_graph(*graph);
            }
            msc::store_graph(*graph, this->parameters.output);
            msc::print_meta(get_info(*graph, this->parameters.output), this->parameters.meta);
        } else {
            MSC_NOT_REACHED();
        }
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Imports a graph or layout file from an external source.");
    return app(argc, argv);
}
