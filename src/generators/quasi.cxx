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

#include <cassert>
#include <cmath>
#include <map>
#include <memory>
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
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "projection.hxx"
#include "random.hxx"
#include "strings.hxx"

#define PROGRAM_NAME "quasi"

namespace /*anonymous*/
{

    template <std::size_t N>
    using grid_node_map = std::map<msc::point<double, N>, ogdf::node, msc::point_order<double>>;

    template <std::size_t N>
    void add_gird_edges(ogdf::Graph& graph, const grid_node_map<N>& nodemap)
    {
        for (const auto [grid, node] : nodemap) {
            for (std::size_t dim = 0; dim < N; ++dim) {
                for (auto off : {-1.0, +1.0}) {
                    auto next = grid;
                    next[dim] += off;
                    const auto pos = nodemap.find(next);
                    if (pos != nodemap.end()) {
                        const auto other = pos->second;
                        if (graph.searchEdge(node, other) == nullptr) {
                            graph.newEdge(node, other);
                        }
                    }
                }
            }
        }
    }

    template <std::size_t N>
    bool advance_grid_point(msc::point<double, N>& p, const double limit) noexcept
    {
        for (std::size_t i = N; i > 0; --i) {
            if (p[i - 1] < limit) {
                p[i - 1] += 1.0;
                return true;
            }
            p[i - 1] = 0.0;
        }
        return false;
    }

    template <std::size_t N>
    constexpr msc::point<double, N>
    project_point(const msc::point<double, N>& p, const msc::point<double, N>& n) noexcept
    {
        return p - dot(n, p) * n;
    }

    template <std::size_t N>
    msc::point<double, N> round_point(msc::point<double, N> p) noexcept
    {
        for (auto& x : p) {
            x = std::round(x);
        }
        return p;
    }

    template <std::size_t N>
    std::tuple<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_quasi(const msc::point<double, N>& normal,
               const msc::point<double, N>& e1,
               const msc::point<double, N>& e2,
               const double size,
               const double thickness)
    {
        using point_nd = msc::point<double, N>;
        const auto features = ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics;
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph, features);
        attrs->directed() = false;
        auto nodes = grid_node_map<N>{};
        const auto agite = [&](const point_nd& grid){
            const auto proj = project_point(grid, normal);
            if ((distance(proj, grid) <= thickness) && !nodes.count(grid)) {
                const auto v = graph->newNode();
                const auto p = msc::transform2d(proj, e1, e2);
                attrs->x(v) = p.x();
                attrs->y(v) = p.y();
                nodes[grid] = v;
            }
        };
        for (auto r1 = 0.0; r1 <= size; r1 += 1.0) {
            for (auto r2 = 0.0; r2 <= size; r2 += 1.0) {
                const auto grid = round_point(r1 * e1 + r2 * e2);
                agite(grid);
                for (std::size_t d = 0; d < N; ++d) {
                    agite(grid + msc::make_unit_point<double, N>(d));
                    agite(grid - msc::make_unit_point<double, N>(d));
                }
            }
        }
        add_gird_edges(*graph, nodes);
        msc::normalize_layout(*attrs);
        return {std::move(graph), std::move(attrs)};
    }

    template <std::size_t N, typename EngineT>
    std::tuple<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_quasi(EngineT& engine, const int nodes)
    {
        auto vecdist = std::uniform_real_distribution<double>{-1.0, +1.0};
        const auto thickness = std::uniform_real_distribution{0.1, 1.1}(engine);
        const auto size = std::max(1.0, std::sqrt(nodes));
        const auto normal = msc::make_random_point<double, N>(engine, vecdist);
        const auto randompoint = [normal, &engine, &vecdist](){
            return project_point(msc::make_random_point<double, N>(engine, vecdist), normal);
        };
        const auto v1 = randompoint();
        const auto v2 = randompoint();
        const auto e1 = msc::normalized(v1);
        const auto e2 = msc::normalized(v2 - e1 * dot(e1, v2));
        assert(std::abs(dot(e1, e2)) < 1.0E-10);
        return make_quasi(normal, e1, e2, size, thickness);
    }

    template <typename EngineT>
    std::tuple<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_quasi(EngineT& engine, const int nodes, const int hyperdim)
    {
        switch (hyperdim) {
        case 2: return make_quasi<2>(engine, nodes);
        case 3: return make_quasi<3>(engine, nodes);
        case 4: return make_quasi<4>(engine, nodes);
        case 5: return make_quasi<5>(engine, nodes);
        case 6: return make_quasi<6>(engine, nodes);
        default: throw std::invalid_argument{std::to_string(hyperdim) + "-dimensional hyper spaces are not supported"};
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
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["graph"] = msc::graph_fingerprint(attrs.constGraph());
        info["layout"] = msc::layout_fingerprint(attrs);
        info["nodes"] = msc::json_diff{attrs.constGraph().numberOfNodes()};
        info["edges"] = msc::json_diff{attrs.constGraph().numberOfEdges()};
        info["native"] = msc::json_bool{true};
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["seed"] = seed;
        info["filename"] = msc::make_json(dst.filename());
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    struct mea_culpa : std::runtime_error
    {
        mea_culpa(const std::string_view why) : std::runtime_error{msc::concat("Sorry, I've messed up: ", why)}
        {
        }
    };

    void application::operator()() const
    {
        auto engine = std::mt19937{};
        const auto seed = msc::seed_random_engine(engine);
        const auto [graph, attrs] = make_quasi(engine, this->parameters.nodes, this->parameters.hyperdim);
        if (graph->numberOfNodes() > this->parameters.nodes * 10) {
            throw mea_culpa{"Graph contains more than 10 x the number of desired nodes"};
        }
        if (graph->numberOfNodes() < this->parameters.nodes / 10) {
            throw mea_culpa{"Graph contains less than 1 / 10 the number of desired nodes"};
        }
        if (graph->numberOfEdges() < graph->numberOfNodes() / 2) {
            throw mea_culpa{"Graph is highly disconnected"};
        }
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generates a random graph with a layout of a 2-dimensional quasi crystal.");
    return app(argc, argv);
}
