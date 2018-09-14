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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "enums/projections.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "math_constants.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "projection.hxx"
#include "random.hxx"

#define PROGRAM_NAME "bottle"

namespace /*anonymous*/
{

    void wire_segment(ogdf::Graph& graph, const std::vector<ogdf::node>& nodes)
    {
        const auto n = nodes.size();
        for (std::size_t i = 1; i <= n; ++i) {
            graph.newEdge(nodes[i - 1], nodes[(i == n) ? 0 : i]);
        }
    }

    void wire_segments_together(ogdf::Graph& graph,
                                const std::vector<ogdf::node>& first,
                                const std::vector<ogdf::node>& second)
    {
        if (first.empty() || second.empty()) {
            return;
        }
        const auto n = first.size();
        const auto m = second.size();
        const auto ratio = static_cast<double>(m) / static_cast<double>(n);
        for (std::size_t i = 0; i < n; ++i) {
            const auto j0 = static_cast<std::size_t>(std::round(i * ratio)) % m;
            const auto j1 = (m + j0 - 1) % m;
            const auto j2 = (m + j0 + 1) % m;
            std::size_t indices[] = {j0, j1, j2};
            std::sort(std::begin(indices), std::end(indices));
            const auto lastit = std::unique(std::begin(indices), std::end(indices));
            for (auto it = std::begin(indices); it != lastit; ++it) {
                graph.newEdge(first.at(i), second.at(*it));
            }
        }
    }

    template <typename FuncT>
    double get_next_z(const FuncT& radius, const double old)
    {
        const auto target = 1.0;
        const auto tolerance = 1.0E-2;
        const auto prev = msc::point2d{old, radius(old)};
        const auto f = [prev, &radius](const double z){ return distance(prev, msc::point2d{z, radius(z)}); };
        auto lo = old;
        auto hi = old + target;
        for (auto i = 0; i < 100; ++i) {
            assert(f(lo) <= target);
            assert(f(hi) >= target);
            const auto z = (lo + hi) / 2.0;
            const auto actual = f(z);
            if (std::abs(actual - target) <= tolerance) {
                return z;
            } else if (actual < target) {
                lo = z;
            } else if (actual > target) {
                hi = z;
            } else {
                std::terminate();
            }
        }
        throw std::runtime_error{"Discontinuous radius function"};
    }

    template <typename FuncT>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_bottle(const FuncT& radius, const double length, const msc::projections proj)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        auto coords = ogdf::NodeArray<msc::point3d>{*graph};
        auto previous = std::vector<ogdf::node>{};
        auto segment = std::vector<ogdf::node>{};
        for (auto z = 0.0; z < length; z = get_next_z(radius, z)) {
            const auto r = radius(z);
            const auto n = std::ceil(2.0 * r * M_PI);
            for (auto i = 0.0; i <= n; i += 1.0) {
                const auto alpha = i * 2.0 * M_PI / (n + 1.0);
                const auto x = r * std::sin(alpha);
                const auto y = r * std::cos(alpha);
                const auto v = graph->newNode();
                segment.push_back(v);
                coords[v] = msc::point3d{x, y, z};
            }
            wire_segment(*graph, segment);
            wire_segments_together(*graph, segment, previous);
            segment.swap(previous);
            segment.clear();
        }
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        for (const auto v : graph->nodes) {
            const auto [x, y] = msc::axonometric_projection(proj, coords[v]);
            attrs->x(v) = x;
            attrs->y(v) = y;
        }
        return {std::move(graph), std::move(attrs)};
    }

    constexpr double sqr(const double x) noexcept
    {
        return x * x;
    }

    template <typename EngineT>
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_graph_and_layout(EngineT& rndeng, const int nodes, const msc::projections proj)
    {
        constexpr auto ncoeffs = 10;
        const auto sqrtn = std::sqrt(nodes);
        const auto r = std::uniform_real_distribution<double>{0.0, 0.5 * sqrtn}(rndeng);
        const auto l = std::uniform_real_distribution<double>{2.0 * r, 2.0 * sqrtn}(rndeng);
        auto rnddst = std::uniform_real_distribution<double>{0.0, 1.0 / ncoeffs};
        double coeffs[ncoeffs];
        std::generate(std::begin(coeffs), std::end(coeffs), [&rndeng, &rnddst](){ return rnddst(rndeng); });
        const auto basic_radius = [l, r](const double z){
            if ((z < 0.0) || (z > l)) {
                return 0.0;
            };
            const auto closest = std::min(z, l - z);
            if (closest < r) { return std::sqrt(sqr(r) - sqr(r - closest)); }
            return r;
        };
        const auto radius = [basic_radius, l, first = std::begin(coeffs), last = std::end(coeffs)](const double z){
            auto factor = 1.0;
            for (auto it = first; it != last; ++it) {
                const auto overtone = 1.0 + (it - first);
                const auto coeff = *it;
                factor += coeff * std::sin(M_PI * overtone * z / l);
            }
            return factor * basic_radius(z);
        };
        return make_bottle(radius, l, proj);
    }

    struct cli_parameters
    {
        msc::output_file output{"-"};
        msc::output_file meta{};
        int nodes{1000};
        msc::projections projection{msc::projections::isometric};
    };

    struct application
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs,
                              const std::string& seed,
                              const msc::projections proj,
                              const msc::output_file& dst)
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
        info["projection"] = name(proj);
        info["filename"] = msc::make_json(dst.filename());
        info["producer"] = PROGRAM_NAME;
        info["seed"] = seed;
        return info;
    }

    void application::operator()() const
    {
        auto engine = std::mt19937{};
        const auto seed = msc::seed_random_engine(engine);
        auto [graph, attrs] = make_graph_and_layout(engine, this->parameters.nodes, this->parameters.projection);
        msc::normalize_layout(*attrs);
        const auto info = get_info(*attrs, seed, this->parameters.projection, this->parameters.output);
        msc::store_layout(*attrs, this->parameters.output);
        msc::print_meta(info, this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generates a graph with a native layout that looks like a bottle if you squint.");
    return app(argc, argv);
}
