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
#include <cassert>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

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
#include "useful.hxx"

#define PROGRAM_NAME "lindenmayer"

namespace /*anonymous*/
{

    using prg_type = std::ranlux48_base;

    constexpr auto sub_radius_factor = 2.0 / 5.0;

    msc::point2d get_coords(const ogdf::GraphAttributes& attrs, const ogdf::node vertex)
    {
        return {attrs.x(vertex), attrs.y(vertex)};
    }

    void set_coords(ogdf::GraphAttributes& attrs, const ogdf::node vertex, const msc::point2d coords)
    {
        attrs.x(vertex) = coords.x();
        attrs.y(vertex) = coords.y();
    }

    double polar(const msc::point2d diff)
    {
        return std::atan2(diff.x(), diff.y());
    }

    double polar(const ogdf::GraphAttributes& attrs, const msc::point2d center, const ogdf::node vertex)
    {
        return polar(get_coords(attrs, vertex) - center);
    }

    msc::point2d cartesian(const double phi) noexcept
    {
        return {std::sin(phi), std::cos(phi)};
    }

    std::vector<ogdf::node> get_adjacent_vertices(const ogdf::GraphAttributes& attrs, const ogdf::node v)
    {
        auto vertices = std::vector<ogdf::node>{};
        vertices.reserve(v->degree());
        for (const auto adj : v->adjEntries) {
            vertices.push_back(adj->twinNode());
        }
        const auto comp = [&attrs, center = get_coords(attrs, v)](const ogdf::node u1, const ogdf::node u2){
            return polar(attrs, center, u1) < polar(attrs, center, u2);
        };
        std::sort(std::begin(vertices), std::end(vertices), comp);
        return vertices;
    }

    double find_radius(const ogdf::GraphAttributes& attrs, const ogdf::node v)
    {
        auto closest = std::numeric_limits<double>::infinity();
        const auto center = get_coords(attrs, v);
        for (const auto& adj : v->adjEntries) {
            const auto p = get_coords(attrs, adj->twinNode());
            closest = std::min(closest, distance(center, p));
        }
        return closest;
    }

    class l_radial
    {
    protected:

        static constexpr unsigned f_none         = 0u;
        static constexpr unsigned f_center_node  = (1u << 0);
        static constexpr unsigned f_ring_edges   = (1u << 1);
        static constexpr unsigned f_clique_edges = (1u << 2);

        l_radial(ogdf::Graph& graph, ogdf::GraphAttributes& attrs, const unsigned features) :
            _graph{&graph}, _attrs{&attrs}, _features{features}
        {
        }

    public:

        std::vector<ogdf::node> operator()(const ogdf::node vertex, const int steps, const double radius)
        {
            const auto arity = vertex->degree();
            const auto adjacent = get_adjacent_vertices(*_attrs, vertex);
            const auto center = get_coords(*_attrs, vertex);
            _graph->delNode(vertex);
            auto vertices = (arity > 0)
                ? _radial_insert(adjacent, steps, center, radius)
                : _radial_insert(steps, center, radius);
            if (_features & f_clique_edges) {
                for (std::size_t i = 1; i < vertices.size(); ++i) {
                    _graph->newEdge(vertices[i - 1], vertices[i]);
                }
                if (vertices.size() > 2) {
                    _graph->newEdge(vertices.back(), vertices.front());
                }
            } else if (_features & f_ring_edges) {
                for (auto it1 = std::begin(vertices); it1 != std::end(vertices); ++it1) {
                    for (auto it2 = std::next(it1); it2 != std::end(vertices); ++it2) {
                        _graph->newEdge(*it1, *it2);
                    }
                }
            }
            if (_features & f_center_node) {
                const auto v = _graph->newNode();
                for (const auto u : vertices) {
                    _graph->newEdge(v, u);
                }
                vertices.push_back(v);
                set_coords(*_attrs, v, center);
                _attrs->shape(v) = ogdf::Shape::Trapeze;
            }
            return vertices;
        }

    private:

        ogdf::Graph* _graph{};
        ogdf::GraphAttributes* _attrs{};
        unsigned _features{};

        std::vector<ogdf::node> _radial_insert(const std::vector<ogdf::node>& connectors,
                                               const int steps,
                                               const msc::point2d center,
                                               const double radius)
        {
            assert(!connectors.empty());
            assert(steps > 0);
            const auto nodes = steps * static_cast<int>(connectors.size());
            auto vertices = std::vector<ogdf::node>();
            vertices.reserve(nodes);
            for (auto it = std::begin(connectors); it != std::end(connectors); ++it) {
                const auto v1 = *it;
                const auto v2 = *msc::cyclic_next(it, std::cbegin(connectors), std::cend(connectors));
                const auto phi1 = polar(*_attrs, center, v1);
                const auto phi2 = polar(*_attrs, center, v2) + ((v2 == *std::begin(connectors)) ? 2.0 * M_PI : 0);
                {
                    const auto u = _graph->newNode();
                    _graph->newEdge(u, v1);
                    vertices.push_back(u);
                    set_coords(*_attrs, u, center + radius * cartesian(phi1));
                    _attrs->shape(u) = ogdf::Shape::Rect;
                }
                for (auto i = 1; i < steps; ++i) {
                    const auto t = static_cast<double>(i) / static_cast<double>(steps);
                    const auto phi = (1.0 - t) * phi1 + t * phi2;
                    const auto u = _graph->newNode();
                    vertices.push_back(u);
                    set_coords(*_attrs, u, center + radius * cartesian(phi));
                    _attrs->shape(u) = ogdf::Shape::Rhomb;
                }
            }
            return vertices;
        }

        std::vector<ogdf::node> _radial_insert(const int nodes, const msc::point2d center, const double radius)
        {
            auto vertices = std::vector<ogdf::node>(nodes);
            for (auto i = 0; i < nodes; ++i) {
                const auto v = vertices[i] = _graph->newNode();
                const auto alpha = 2.0 * M_PI * i / nodes;
                set_coords(*_attrs, v, center + radius * msc::point2d{std::sin(alpha), std::cos(alpha)});
                _attrs->shape(v) = ogdf::Shape::Rhomb;
            }
            return vertices;
        }

    };  // class l_radial

    struct l_star final : l_radial
    {
        l_star(ogdf::Graph& graph, ogdf::GraphAttributes& attrs)
            : l_radial{graph, attrs, l_radial::f_center_node}
        { }
    };

    struct l_ring final : l_radial
    {
        l_ring(ogdf::Graph& graph, ogdf::GraphAttributes& attrs)
            : l_radial{graph, attrs, l_radial::f_ring_edges}
        { }
    };

    struct l_wheel final : l_radial
    {
        l_wheel(ogdf::Graph& graph, ogdf::GraphAttributes& attrs)
            : l_radial{graph, attrs, l_radial::f_center_node | l_radial::f_ring_edges}
        { }
    };

    struct l_clique final : l_radial
    {
        l_clique(ogdf::Graph& graph, ogdf::GraphAttributes& attrs)
            : l_radial{graph, attrs, l_radial::f_clique_edges}
        { }
    };

    class l_grid final
    {
    public:

        l_grid(ogdf::Graph& graph, ogdf::GraphAttributes& attrs) : _graph{&graph}, _attrs{&attrs}
        {
        }

        std::vector<ogdf::node> operator()(const ogdf::node vertex, const int n, const int m, const double radius)
        {
            assert(n > 0);
            assert(m > 0);
            const auto corner = get_coords(*_attrs, vertex) - radius * msc::point2d{0.5, 0.5};
            _graph->delNode(vertex);
            auto vertices = std::vector<ogdf::node>{};
            vertices.reserve(n * m);
            for (auto i = 0; i < n; ++i) {
                for (auto j = 0; j < m; ++j) {
                    const auto v = _graph->newNode();
                    if (i > 0) { _graph->newEdge(vertices.at(m * (i - 1) + j), v); }
                    if (j > 0) { _graph->newEdge(vertices.back(), v); }
                    vertices.push_back(v);
                    set_coords(*_attrs, v, corner + radius * msc::point2d{1.0 * i / n, 1.0 * j / m});
                    switch (int((i == 0) || (i + 1 == n)) + int((j == 0) || (j + 1 == m))) {
                    case 0:
                        _attrs->shape(v) = ogdf::Shape::Rect;
                        break;
                    case 1:
                        _attrs->shape(v) = ogdf::Shape::Rhomb;
                        break;
                    case 2:
                        _attrs->shape(v) = ogdf::Shape::Trapeze;
                        break;
                    default:
                        MSC_NOT_REACHED();
                    }
                }
            }
            return vertices;
        }

    private:

        ogdf::Graph* _graph{};
        ogdf::GraphAttributes* _attrs{};

    };  // class l_grid

    class l_singleton final
    {
    public:

        l_singleton(ogdf::Graph& graph, ogdf::GraphAttributes& attrs) : _graph{&graph}, _attrs{&attrs}
        {
        }

        std::vector<ogdf::node> operator()(const ogdf::node vertex)
        {
            _attrs->shape(vertex) = ogdf::Shape::Rect;
            return {vertex};
        }

    private:

        ogdf::Graph* _graph{};
        ogdf::GraphAttributes* _attrs{};

    };  // class l_grid

    class lindenworker final
    {
    public:

        lindenworker(ogdf::Graph& graph, ogdf::GraphAttributes& attrs) : _graph{&graph}, _attrs{&attrs}
        {
        }

        void operator()(prg_type& prg, const ogdf::node v, const int size, const double radius = 1000.0)
        {
            const auto token = std::string{"/"};
            _radii[token] = radius;
            _recurse(prg, v, size, token);
        }

    private:

        ogdf::Graph* _graph{};
        ogdf::GraphAttributes* _attrs{};
        std::map<std::string, double> _radii{};

        void _recurse(prg_type& prg, const ogdf::node v, const int size, const std::string& token)
        {
            const auto radius = _get_radius(v, token);
            const auto new_vertices = _apply_sub_generator(prg, v, size, radius);
            const auto count = static_cast<int>(new_vertices.size());
            const auto subsize = (size - count + 1) / count;
            if (subsize > 0) {
                for (const auto u : new_vertices) {
                    auto subprg = prg;
                    subprg.discard(static_cast<int>(_attrs->shape(u)));
                    const auto subtoken = token + msc::random_hex_string(subprg, 8) + "/";
                    _recurse(subprg, u, subsize, subtoken);
                }
            }
        }

        double _get_radius(const ogdf::node v, const std::string& token)
        {
            if (const auto pos = _radii.find(token); pos != _radii.cend()) {
                return pos->second;
            }
            return _radii[token] = std::min(_radii.at("/"), find_radius(*_attrs, v)) / 4.0;
        }

        std::vector<ogdf::node>
        _apply_sub_generator(prg_type& prg, const ogdf::node v, const int size, const double radius)
        {
            const auto magic = std::uniform_real_distribution{0.0, 1.0}(prg);
            if ((v->degree() == 0) && (magic < 0.25)) {
                auto dist = std::uniform_int_distribution{2, std::max(2, static_cast<int>(std::sqrt(radius / 15.0)))};
                const auto n = dist(prg);
                const auto m = dist(prg);
                return l_grid{*_graph, *_attrs}(v, n, m, radius);
            }
            const auto k = [&](){
                const auto kmin = (v->degree() > 1) ? 1 : 2;
                const auto kmax1 = size / std::max(1, v->degree());
                const auto kmax2 = std::max(1, static_cast<int>(std::sqrt(size)));
                const auto kmax = std::max(kmin, std::min(kmax1, kmax2));
                return std::uniform_int_distribution{kmin, kmax}(prg);
            }();
            if (magic < 1.0 / std::max(5, v->degree())) {
                return l_clique{*_graph, *_attrs}(v, k, radius);
            } else if (magic < 2.0 / 5) {
                return l_wheel{*_graph, *_attrs}(v, k, radius);
            } else if (magic < 3.0 / 5) {
                return l_ring{*_graph, *_attrs}(v, k, radius);
            } else if (magic < 4.0 / 5) {
                return l_star{*_graph, *_attrs}(v, k, radius);
            } else {
                return l_singleton{*_graph, *_attrs}(v);
            }
        }

    };  // class lindenworker

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_lindenmayer(prg_type& engine, const int nodes)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        const auto v0 = graph->newNode();
        attrs->x(v0) = 0.0;
        attrs->y(v0) = 0.0;
        auto lw = lindenworker{*graph, *attrs};
        lw(engine, v0, nodes);
        msc::normalize_layout(*attrs);
        return {std::move(graph), std::move(attrs)};
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

    void application::operator()() const
    {
        auto engine = prg_type{};
        const auto seed = msc::seed_random_engine(engine);
        const auto [graph, attrs] = make_lindenmayer(engine, this->parameters.nodes);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generate a random kinda symmetric graph using a stocastic Lindenmayer system.");
    return app(argc, argv);
}
