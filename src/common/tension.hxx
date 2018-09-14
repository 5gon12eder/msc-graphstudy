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
 * @file rdf.hxx
 *
 * @brief
 *     Shared functionality for computing tension.
 *
 * @warning
 *     This header actually includes headers from the OGDF rather than just forward-declaring some types.
 *
 */

#ifndef MSC_TENSION_HXX
#define MSC_TENSION_HXX

#include <ogdf/basic/GraphAttributes.h>

#include "pairwise.hxx"

namespace msc
{

    /**
     * @brief
     *     Projection of node pairs to the quotient of their Euclidian distance in the layout and graph-theoretical
     *     distance.
     *
     */
    struct node_tension
    {

        /**
         * @brief
         *     Non-throwing default constructor.
         *
         * @warning
         *     A default-constructed `node_tension` projection must never be called.
         *
         */
        node_tension() noexcept = default;

        /**
         * @brief
         *     Constructor that is actually useful.
         *
         * The behavior is undefined if `attrs` and `matrix` do not refer to the same graph
         *
         * @param attrs
         *     graph attributes of the layout that defines the Euclidian distances
         *
         * @param matrix
         *     shortest path matrix
         *
         */
        node_tension(const ogdf::GraphAttributes& attrs, const ogdf_node_array_2d<double>& matrix) noexcept
            : _attrs{&attrs}, _matrix{&matrix}
        {
        }

        /**
         * @brief
         *     Returns the tension between two nodes.
         *
         * The behavior is undefined if `v1 == v2`, either of `v1` or `v2` is `nullptr` or not from
         * `attrs.constGraph().nodes`.  Furthermore, the behavior is undefined unless `attrs` has 2D coorindates
         * assigned to the nodes.
         *
         * @param v1
         *     first node
         *
         * @param v2
         *     second node
         *
         */
        double operator()(ogdf::node v1, ogdf::node v2) const noexcept;

    private:

        /** @brief Referenced layout. */
        const ogdf::GraphAttributes* _attrs{};

        /** @brief Referenced shortest path matrix.  */
        const ogdf_node_array_2d<double>* _matrix{};

    };  // struct node_tension

    /**
     * @brief
     *     Presents the range of quotients of Euclidian distance in the layout and graph-theoretical distance between
     *     connected nodes.
     *
     * The referenced layout and shortest path matrix must remain valid (and not be moved) while this range is used.
     *
     */
    class pairwise_tension final
    {
    public:

        /** @brief The exposed iterator type. */
        using iterator = node_pair_iterator<double, threshold_node_pair_predicate<double>, node_tension>;

        /**
         * @brief
         *     Creates a range from the given layout.
         *
         * The behavior is undefined if the matrix does not correspond to the graph of the layout.
         *
         * @param attrs
         *     layout to view the distances in
         *
         * @param matrix
         *     shortest path matrix
         *
         * @param infinity
         *     value that is larger than the longest shortest path in the graph
         *
         */
        pairwise_tension(const ogdf::GraphAttributes& attrs,
                         const ogdf_node_array_2d<double>& matrix,
                         const double infinity) noexcept
            : _attrs{&attrs}, _matrix{&matrix}, _infty{infinity}
        {
        }

        /**
         * @brief
         *     Returns an iterator to the first quotient.
         *
         * @returns
         *     iterator to first quotient
         *
         */
        iterator begin() const noexcept
        {
            const auto pred = threshold_node_pair_predicate<double>{*_matrix, _infty};
            const auto proj = node_tension{*_attrs, *_matrix};
            return iterator{_attrs->constGraph(), pred, proj};
        }

        /**
         * @brief
         *     Returns an iterator after the last quotient.
         *
         * @returns
         *     iterator after last quotient
         *
         */
        iterator end() const noexcept
        {
            return iterator{};
        }

    private:

        /** @brief Referenced graph layout.  */
        const ogdf::GraphAttributes* _attrs{};

        /** @brief Referenced shortest path matrix.  */
        const ogdf_node_array_2d<double>* _matrix{};

        /** Value larger than the longest shortest path (between connected nodes) in the graph.  */
        double _infty{};

    };  // class pairwise_tension

}  // namespace msc

#define MSC_INCLUDED_FROM_TENSION_HXX
#include "tension.txx"
#undef MSC_INCLUDED_FROM_TENSION_HXX

#endif  // !defined(MSC_TENSION_HXX)
