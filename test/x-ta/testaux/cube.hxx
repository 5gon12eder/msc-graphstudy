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

/**
 * @file cube.hxx
 *
 * @brief
 *     Cube graphs for testing purposes.
 *
 */

#ifndef MSC_TESTAUX_CUBE_HXX
#define MSC_TESTAUX_CUBE_HXX

#include <memory>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

namespace msc::test
{

    /**
     * @brief
     *     Returns a graph that is a square.
     *
     * @returns
     *     the graph
     *
     */
    std::unique_ptr<ogdf::Graph> make_square_graph();

    /**
     * @brief
     *     Returns a layouted graph that is a square with an edge length of 100.
     *
     * @returns
     *     a pair of graph and layout
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>> make_square_layout();

    /**
     * @brief
     *     Returns a graph that is a cube.
     *
     * @returns
     *     `make_me_a_darn_cube().first`
     *
     */
    std::unique_ptr<ogdf::Graph> make_cube_graph();

    /**
     * @brief
     *     Returns a layouted graph that is a cube with an edge length of 100 and 50 respectively.
     *
     * @returns
     *     a pair of graph and layout
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>> make_cube_layout();

    /**
     * @brief
     *     Generates a graph with the given number of nodes and edges.
     *
     * @param n
     *     desired number of nodes
     *
     * @param m
     *     desired number of edges
     *
     * @param seed
     *     random seed
     *
     * @returns
     *     the graph
     *
     */
    std::unique_ptr<ogdf::Graph> make_test_graph(int n, int m, const char *seed = nullptr);

    /**
     * @brief
     *     Generates a graph and layout with the given number of nodes and edges.
     *
     * The layout will be very ugly.
     *
     * @param n
     *     desired number of nodes
     *
     * @param m
     *     desired number of edges
     *
     * @param seed
     *     random seed
     *
     * @returns
     *     a pair of graph and layout
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_test_layout(int n, int m, const char *seed = nullptr);

}  // namespace msc::test

#endif  // !defined(MSC_TESTAUX_CUBE_HXX)
