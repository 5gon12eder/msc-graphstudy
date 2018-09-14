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
#include <exception>
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

#define PROGRAM_NAME "mosaic"

namespace /*anonymous*/
{

    // TODO: This could be a template that works in 2d and 3d alike.
    class generator final
    {
    public:

        template <typename EngineT>
        std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
        operator()(const int nodes, EngineT& engine, const bool symmetric)
        {
            _graph = std::make_unique<ogdf::Graph>();
            _attrs = std::make_unique<ogdf::GraphAttributes>(*_graph);
            _splitedges.clear();
            _leafshapes.clear();
            _indishapes.clear();
            _make_initial_simplex(std::max(3, nodes), engine);
            if (symmetric) {
                while (_graph->numberOfNodes() < nodes / 4) {
                    _break_all_shapes(engine);
                }
            } else {
                while (_graph->numberOfNodes() < nodes) {
                    _break_another_shape(engine);
                }
            }
            return {std::move(_graph), std::move(_attrs)};
        }

    private:

        std::unique_ptr<ogdf::Graph> _graph{};
        std::unique_ptr<ogdf::GraphAttributes> _attrs{};
        std::map<std::pair<ogdf::node, ogdf::node>, ogdf::node> _splitedges{};
        std::vector<std::vector<ogdf::node>> _leafshapes{};
        std::vector<std::vector<ogdf::node>> _indishapes{};

        template <typename EngineT>
        void _make_initial_simplex(const int maxn, EngineT& engine)
        {
            const auto p = std::min(1.0, 3.0 / std::cbrt(maxn));
            const auto n = std::clamp(std::geometric_distribution<int>{p}(engine), 3, maxn);
            auto nodes = std::vector<ogdf::node>{};
            for (auto i = 0; i < n; ++i) {
                const auto v = nodes.emplace_back(_graph->newNode());
                const auto theta = i * 2.0 * M_PI / n;
                _attrs->x(v) = 1.0E6 * std::sin(theta);
                _attrs->y(v) = 1.0E6 * std::cos(theta);
            }
            for (auto i = 0; i < n; ++i) {
                _new_edge(nodes[i], nodes[(i + 1) % n]);
            }
            _put_leaf(std::move(nodes));
        }

        template <typename EngineT>
        void _break_all_shapes(EngineT& engine)
        {
            auto leafs = std::vector<std::vector<ogdf::node>>{};
            auto indis = std::vector<std::vector<ogdf::node>>{};
            leafs.swap(_leafshapes);
            indis.swap(_indishapes);
            for (const auto shapes : {&leafs, &indis}) {
                const auto first = std::begin(*shapes);
                const auto last = std::end(*shapes);
                switch (std::uniform_int_distribution<int>{1, 4}(engine)) {
                case 1:
                    std::for_each(first, last, [this](const auto& nodes){ _break_star(nodes); });
                    break;
                case 2:
                    std::for_each(first, last, [this](const auto& nodes){ _break_flower(nodes); });
                    break;
                case 3:
                    std::for_each(first, last, [this](const auto& nodes){ _break_shape(nodes); });
                    break;
                case 4:
                    std::for_each(first, last, [this](const auto& nodes){ _break_nothing(nodes); });
                    break;
                default:
                    std::terminate();
                }
            }
        }

        template <typename EngineT>
        void _break_another_shape(EngineT& engine)
        {
            const auto count = _leafshapes.size() + _indishapes.size();
            auto idx = std::uniform_int_distribution<std::size_t>{0, count - 1}(engine);
            auto nodes = std::vector<ogdf::node>{};
            if (idx < _leafshapes.size()) {
                _leafshapes.back().swap(_leafshapes[idx]);
                nodes.swap(_leafshapes.back());
                _leafshapes.pop_back();
            } else {
                _indishapes.back().swap(_indishapes[idx - _leafshapes.size()]);
                nodes.swap(_indishapes.back());
                _indishapes.pop_back();
            }
            switch (std::uniform_int_distribution<int>{1, 3}(engine)) {
            case 1: return _break_star(nodes);
            case 2: return _break_flower(nodes);
            case 3: return _break_shape(nodes);
            default: std::terminate();
            }
        }

        void _break_star(const std::vector<ogdf::node>& nodes)
        {
            const auto n = nodes.size();
            const auto center = _get_center(nodes);
            const auto v = _graph->newNode();
            _attrs->x(v) = center.x();
            _attrs->y(v) = center.y();
            for (auto i = std::size_t{1}; i <= n; ++i) {
                const auto v1 = nodes[i - 1];
                const auto v2 = nodes[(i == n) ? 0 : i];
                _new_edge(v1, v);
                _put_leaf({v1, v, v2});
            }
        }

        void _break_flower(const std::vector<ogdf::node>& nodes)
        {
            const auto n = nodes.size();
            const auto center = _get_center(nodes);
            const auto v = _graph->newNode();
            _attrs->x(v) = center.x();
            _attrs->y(v) = center.y();
            auto added = std::vector<ogdf::node>{};
            added.reserve(n);
            for (auto i = std::size_t{1}; i <= n; ++i) {
                const auto v1 = nodes[i - 1];
                const auto v2 = nodes[(i == n) ? 0 : i];
                added.push_back(_split(v1, v2));
            }
            for (auto i = std::size_t{0}; i < n; ++i) {
                const auto w = nodes[i];
                const auto u1 = added[i];
                const auto u2 = added[(i + n - 1) % n];
                _new_edge(u1, v);
                _put_leaf({u1, w, u2, v});
            }
        }

        void _break_shape(const std::vector<ogdf::node>& nodes)
        {
            const auto n = nodes.size();
            auto added = std::vector<ogdf::node>{};
            auto indi = std::vector<ogdf::node>{};
            for (auto i = std::size_t{1}; i <= n; ++i) {
                const auto v1 = nodes[i - 1];
                const auto v2 = nodes[(i == n) ? 0 : i];
                const auto v = _split(v1, v2);
                added.push_back(v);
                indi.push_back(v);
            }
            for (auto i = std::size_t{0}; i < n; ++i) {
                const auto w = nodes[i];
                const auto u1 = added[i];
                const auto u2 = added[(i + n - 1) % n];
                _new_edge(u1, u2);
                _put_leaf({u1, w, u2});
            }
            _put_indi(indi);
        }

        void _break_nothing(const std::vector<ogdf::node>& nodes)
        {
            _put_leaf(nodes);
        }

        ogdf::node _split(const ogdf::node v1, const ogdf::node v2)
        {
            assert(v1 != v2);
            const auto pair = std::less<ogdf::node>{}(v1, v2) ? std::make_pair(v1, v2) : std::make_pair(v2, v1);
            const auto [iter, need] = _splitedges.insert({pair, nullptr});
            if (need) {
                const auto v = _graph->newNode();
                _del_edge(v1, v2);
                _new_edge(v, v1);
                _new_edge(v, v2);
                iter->second = v;
                const auto c1 = msc::point2d{_attrs->x(v1), _attrs->y(v1)};
                const auto c2 = msc::point2d{_attrs->x(v2), _attrs->y(v2)};
                const auto c = 0.5 * c1 + 0.5 * c2;
                _attrs->x(v) = c.x();
                _attrs->y(v) = c.y();
            }
            return iter->second;
        }

        msc::point2d _get_center(const std::vector<ogdf::node>& nodes) const noexcept
        {
            auto center = msc::point2d{};
            for (const auto v : nodes) {
                center += msc::point2d{_attrs->x(v), _attrs->y(v)};
            }
            return center / static_cast<double>(nodes.size());
        }

        void _put_leaf(std::vector<ogdf::node> nodes)
        {
            _leafshapes.push_back(std::move(nodes));
        }

        void _put_indi(std::vector<ogdf::node> nodes)
        {
            _indishapes.push_back(std::move(nodes));
        }

        void _new_edge(const ogdf::node v1, const ogdf::node v2)
        {
            std::less<ogdf::node>{}(v1, v2) ? _graph->newEdge(v1, v2) : _graph->newEdge(v2, v1);
        }

        void _del_edge(const ogdf::node v1, const ogdf::node v2)
        {
            const auto e = std::less<ogdf::node>{}(v1, v2) ? _graph->searchEdge(v1, v2) : _graph->searchEdge(v2, v1);
            assert(e != nullptr);
            _graph->delEdge(e);
        }

    };  // class generator

    template <typename EngineT>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_graph_and_layout(const int nodes, EngineT& engine, const bool symmetric)
    {
        return generator{}(nodes, engine, symmetric);
    }

    struct cli_parameters
    {
        msc::output_file output{"-"};
        msc::output_file meta{};
        int nodes{100};
        bool symmetric{false};
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
        info["layout"] = msc::layout_fingerprint(attrs);
        info["nodes"] = msc::json_diff{attrs.constGraph().numberOfNodes()};
        info["edges"] = msc::json_diff{attrs.constGraph().numberOfEdges()};
        info["native"] = msc::json_bool{true};
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["producer"] = PROGRAM_NAME;
        info["seed"] = seed;
        info["filename"] = msc::make_json(dst.filename());
        return info;
    }

    void application::operator()() const
    {
        auto engine = std::mt19937{};
        const auto seed = msc::seed_random_engine(engine);
        auto [graph, attrs] = make_graph_and_layout(this->parameters.nodes, engine, this->parameters.symmetric);
        msc::normalize_layout(*attrs);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(get_info(*attrs, seed, this->parameters.output), this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generates a random mosaic graph and layout.");
    return app(argc, argv);
}
