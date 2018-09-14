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

#include "testaux/cube.hxx"

#include <climits>
#include <cstring>
#include <random>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/graph_generators.h>

namespace msc::test
{

    namespace /*anonymous*/
    {

        std::mt19937 get_random_engine(const char *const seed = nullptr)
        {
            if (seed == nullptr) {
                auto rnddev = std::random_device{};
                return std::mt19937{rnddev()};
            } else {
                auto seedseq = std::seed_seq(seed, seed + std::strlen(seed));
                return std::mt19937{seedseq};
            }
        }

    }  // namespace /*anonymous*/

    std::unique_ptr<ogdf::Graph> make_square_graph()
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v3, v4);
        graph->newEdge(v4, v1);
        return graph;
    }

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>> make_square_layout()
    {
        const std::pair<int, int> coords[] = {
            {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0},
        };
        auto graph = make_square_graph();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->directed() = false;
        auto idx = 0;
        for (const auto node : graph->nodes) {
            const auto pos = coords[idx++];
            attrs->x(node) = 100.0 * pos.first;
            attrs->y(node) = 100.0 * pos.second;
        }
        return {std::move(graph), std::move(attrs)};
    }

    std::unique_ptr<ogdf::Graph> make_cube_graph()
    {
        const auto edges = ogdf::List<std::pair<int, int>>{
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };
        auto graph = std::make_unique<ogdf::Graph>();
        ogdf::customGraph(*graph, 8, edges);
        return graph;
    }

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>> make_cube_layout()
    {
        const std::pair<int, int> coords[] = {
            {0.0, 0.0}, {1.0, 0.0}, {1.5, 0.5}, {0.5, 0.5},
            {0.0, 1.0}, {1.0, 1.0}, {1.5, 1.5}, {0.5, 1.5},
        };
        auto graph = make_cube_graph();
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        attrs->directed() = false;
        auto idx = 0;
        for (const auto node : graph->nodes) {
            const auto pos = coords[idx++];
            attrs->x(node) = 100.0 * pos.first;
            attrs->y(node) = 100.0 * pos.second;
        }
        return {std::move(graph), std::move(attrs)};
    }

    std::unique_ptr<ogdf::Graph> make_test_graph(const int n, const int m, const char *const seed)
    {
        {
            auto rndeng = get_random_engine(seed);
            auto rnddst = std::uniform_int_distribution<int>{INT_MIN, INT_MAX};
            ogdf::setSeed(rnddst(rndeng));
        }
        auto graph = std::make_unique<ogdf::Graph>();
        ogdf::randomSimpleGraph(*graph, n, m);
        return graph;
    }

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_test_layout(const int n, const int m, const char *const seed)
    {
        auto graph = make_test_graph(n, m, seed);
        auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
        auto rndeng = get_random_engine(seed);
        auto rnddst = std::normal_distribution<double>{0.0, 100.0};
        for (const auto v : graph->nodes) {
            attrs->x(v) = rnddst(rndeng);
            attrs->y(v) = rnddst(rndeng);
        }
        return {std::move(graph), std::move(attrs)};
    }

}  // namespace msc::test
