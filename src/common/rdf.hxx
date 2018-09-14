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
 *     Shared functionality for computing radial distribution functions (RDFs).
 *
 * @warning
 *     This header actually includes headers from the OGDF rather than just forward-declaring some types.
 *
 */

#ifndef MSC_RDF_HXX
#define MSC_RDF_HXX

#include <ogdf/basic/GraphAttributes.h>

#include "pairwise.hxx"

namespace msc
{

    /**
     * @brief
     *     Projection of node pairs to their euclidian distance.
     *
     */
    struct node_distance
    {

        /**
         * @brief
         *     Non-throwing default constructor.
         *
         * @warning
         *     A default-constructed `node_distance` projection must never be called.
         *
         */
        node_distance() noexcept = default;

        /**
         * @brief
         *     Constructor that is actually useful.
         *
         * @param attrs
         *     graph attributes of the layout that defines the Euclidian distances
         *
         */
        node_distance(const ogdf::GraphAttributes& attrs) noexcept : _attrs{&attrs}
        {
        }

        /**
         * @brief
         *     Returns the euclidian distance between two nodes in a given layout.
         *
         * The behavior is undefined if either of `v1` or `v2` is `nullptr` or not from `attrs.constGraph().nodes`.
         * Furthermore, the behavior is undefined unless `attrs` has 2D coorindates assigned to the nodes.
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

    };  // struct node_distance

    /**
     * @brief
     *     Presents the range of all pairwise Euclidian distances in the layout.
     *
     * The referenced layout must remain valid (and not be moved) while this range is used.
     *
     */
    class global_pairwise_distances final
    {
    public:

        /** @brief The exposed iterator type. */
        using iterator = node_pair_iterator<double, tautology_node_pair_predicate, node_distance>;

        /**
         * @brief
         *     Creates a range from the given layout.
         *
         * @param attrs
         *     layout to view the distances in
         *
         */
        explicit global_pairwise_distances(const ogdf::GraphAttributes& attrs) noexcept : _attrs{&attrs}
        {
        }

        /**
         * @brief
         *     Returns an iterator to the first distance.
         *
         * @returns
         *     iterator to first distance
         *
         */
        iterator begin() const noexcept
        {
            const auto pred = tautology_node_pair_predicate{};
            const auto proj = node_distance{*_attrs};
            return iterator{_attrs->constGraph(), pred, proj};
        }

        /**
         * @brief
         *     Returns an iterator after the last distance.
         *
         * @returns
         *     iterator after last distance
         *
         */
        iterator end() const noexcept
        {
            return iterator{};
        }

    private:

        /** @brief Referenced graph layout.  */
        const ogdf::GraphAttributes* _attrs{};

    };  // class global_pairwise_distances

    /**
     * @brief
     *     Presents the range of all pairwise Euclidian distances between nodes (u, v) where the shortest path between u
     *     and v does not exceed a given limit in the layout.
     *
     * The referenced layout ans shortest path matrix must remain valid (and not be moved) while this range is used.
     *
     */
    class local_pairwise_distances final
    {
    public:

        /** @brief The exposed iterator type. */
        using iterator = node_pair_iterator<double, threshold_node_pair_predicate<double>, node_distance>;

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
         * @param limit
         *     longest shortest path to accept
         *
         */
        local_pairwise_distances(const ogdf::GraphAttributes& attrs,
                                 const ogdf_node_array_2d<double>& matrix,
                                 const double limit) noexcept
            : _attrs{&attrs}, _matrix{&matrix}, _limit{limit}
        {
        }

        /**
         * @brief
         *     Returns an iterator to the first distance.
         *
         * @returns
         *     iterator to first distance
         *
         */
        iterator begin() const noexcept
        {
            const auto pred = threshold_node_pair_predicate<double>{*_matrix, _limit};
            const auto proj = node_distance{*_attrs};
            return iterator{_attrs->constGraph(), pred, proj};
        }

        /**
         * @brief
         *     Returns an iterator after the last distance.
         *
         * @returns
         *     iterator after last distance
         *
         */
        iterator end() const noexcept
        {
            return iterator{};
        }

        /**
         * @brief
         *     Returns the longest shortest path that is to be accepted.
         *
         * @returns
         *     longest shortest path
         *
         */
        double limit() const noexcept
        {
            return _limit;
        }

        /**
         * @brief
         *     Sets the longest shortest path that is to be accepted.
         *
         * @param limit
         *     longest shortest path
         *
         */
        void set_limit(const double limit) noexcept
        {
            _limit = limit;
        }

    private:

        /** @brief Referenced graph layout.  */
        const ogdf::GraphAttributes* _attrs{};

        /** @brief Referenced shortest path matrix.  */
        const ogdf_node_array_2d<double>* _matrix{};

        /** @brief Longest shortest path to accept.  */
        double _limit{};

    };  // class local_pairwise_distances

}  // namespace msc

#define MSC_INCLUDED_FROM_RDF_HXX
#include "rdf.txx"
#undef MSC_INCLUDED_FROM_RDF_HXX

#endif  // !defined(MSC_RDF_HXX)
