// -*- coding:utf-8; mode:c++; -*-

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
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "math_constants.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "random.hxx"

#define PROGRAM_NAME "randgeo"

namespace /*anonymous*/
{

    template <typename T, std::size_t... Is>
    auto vecmul_impl(const msc::point<T, sizeof...(Is)>& lhs,
                     const msc::point<T, sizeof...(Is)>& rhs,
                     std::index_sequence<Is...>) noexcept
        -> msc::point<T, sizeof...(Is)>
    {
        using namespace msc;
        return { (get<Is>(lhs) * get<Is>(rhs)) ... };
    }

    template <typename T, std::size_t N>
    auto vecmul(const msc::point<T, N>& lhs, const msc::point<T, N>& rhs) noexcept
    {
        return vecmul_impl(lhs, rhs, std::make_index_sequence<N>());
    }

    template <std::size_t Dim, typename RndEngT>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_random_geometric_graph(RndEngT& engine, const int n)
    {
        using point_type = msc::point<double, Dim>;
        const auto d = M_E / std::max(1.0, std::log(n));
        const auto scale = 0.5 * std::pow(std::max(1, n), 1.0 / Dim);
        auto coorddist = std::uniform_real_distribution{0.0, scale};
        auto scaledist = std::normal_distribution{1.0, 0.125 * scale};
        const auto scalevec = msc::make_random_point<double, Dim>(engine, scaledist);
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        auto points = std::vector<point_type>(n);
        auto vertices = std::vector<ogdf::node>(n);
        for (auto i = 0; i < n; ++i) {
            const auto p = points[i] = vecmul(scalevec, msc::make_random_point<double, Dim>(engine, coorddist));
            const auto v = vertices[i] = graph->newNode();
            attrs->x(v) = p[0];
            attrs->y(v) = p[1];
        }
        for (auto it1 = std::begin(points); it1 != std::end(points); ++it1) {
            const auto p1 = *it1;
            for (auto it2 = std::next(it1); it2 != std::end(points); ++it2) {
                const auto p2 = *it2;
                if (distance(p1, p2) <= d) {
                    graph->newEdge(vertices.at(it1 - std::begin(points)), vertices.at(it2 - std::begin(points)));
                }
            }
        }
        return {std::move(graph), std::move(attrs)};
    }

    template <typename RndEngT>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_random_geometric_graph(RndEngT& engine, const int n, const int dim)
    {
        switch (dim) {
        case 2: return make_random_geometric_graph<2>(engine, n);
        case 3: return make_random_geometric_graph<3>(engine, n);
        case 4: return make_random_geometric_graph<4>(engine, n);
        case 5: return make_random_geometric_graph<5>(engine, n);
        case 6: return make_random_geometric_graph<6>(engine, n);
        default:
            throw std::invalid_argument{"Invalid or unsupported dimensionality of hyper space: " + std::to_string(dim)};
        }
    }

    struct cli_parameters
    {
        msc::output_file output{"-"};
        msc::output_file meta{};
        int nodes{100};
        int hyperdim{3};
    };

    struct application
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const std::string& seed, const msc::output_file& dst)
    {
        auto info = msc::json_object{};
        const auto bbox = msc::get_bounding_box_size(attrs);
        info["graph"] = msc::graph_fingerprint(attrs.constGraph());
        info["nodes"] = msc::json_diff{attrs.constGraph().numberOfNodes()};
        info["edges"] = msc::json_diff{attrs.constGraph().numberOfEdges()};
        info["native"] = msc::json_bool{true};
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
        const auto nodes = std::poisson_distribution{static_cast<double>(this->parameters.nodes)}(rndeng);
        const auto [graph, attrs] = make_random_geometric_graph(rndeng, nodes, this->parameters.hyperdim);
        msc::normalize_layout(*attrs);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back(
        "Generates a random geometric graph using a procedure similar to the one presented by Markus Chimani at GD'18."
    );
    return app(argc, argv);
}
