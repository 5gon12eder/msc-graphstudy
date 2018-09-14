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
 * @file stress.hxx
 *
 * @brief
 *     Shared functionality for computing stress.
 *
 * @warning
 *     This header actually includes headers from the OGDF rather than just forward-declaring some types.
 *
 */

#ifndef MSC_STRESS_HXX
#define MSC_STRESS_HXX

#include <iosfwd>
#include <utility>

#include <ogdf/basic/GraphAttributes.h>

#include "normalizer.hxx"
#include "pairwise.hxx"

namespace msc
{

    /**
     * @brief
     *     Result from fitting a quadratic polynomial and finding its extremal point.
     *
     */
    struct parabola_result
    {
        /** @brief The extremal point.  */
        double x0{};

        /** @brief The extremal value.  */
        double y0{};

        /** @brief The first (degree zero, constant) coefficient of the fitted polynomial.  */
        double a{};

        /** @brief The second (degree one, linear) coefficient of the fitted polynomial.  */
        double b{};

        /** @brief The third (degree three, quadratic) coefficient of the fitted polynomial.  */
        double c{};
    };

    /**
     * @brief
     *     Inserts an informal representation of `pr` into `ostr`.
     *
     * @param ostr
     *     stream to write to
     *
     * @param pr
     *     object to represent
     *
     * @returns
     *     reference to `ostr`
     *
     */
    std::ostream& operator<<(std::ostream& ostr, const parabola_result& pr);

    /**
     * @brief
     *     Computes the stress of a given layout.
     *
     * @param attrs
     *     normalized layout to compute the minmal stress for
     *
     * @param nodesep
     *     desired node separation
     *
     * @returns
     *     stress for the specified node distance
     *
     */
    double compute_stress(const ogdf::GraphAttributes& attrs, double nodesep = default_node_distance);

    /**
     * @brief
     *     Computes the adaptive stress of a given layout.
     *
     * Stress is computed for the node distance that minimizes it.
     *
     * @param attrs
     *     normalized layout to compute the minmal stress for
     *
     * @returns
     *     fitted parabola (the `y0` member contains the stress value at node distance `x0`)
     *
     */
    parabola_result compute_stress_fit_nodesep(const ogdf::GraphAttributes& attrs);

    /**
     * @brief
     *     Computes the adaptive stress of a given layout.
     *
     * Stress is computed for a scaled layout that minimizes stress for the default node separation.
     *
     * @param attrs
     *     normalized layout to compute the minmal stress for
     *
     * @returns
     *     fitted parabola (the `y0` member contains the stress value at scale `x0`)
     *
     */
    parabola_result compute_stress_fit_scale(const ogdf::GraphAttributes& attrs);

    /**
     * @brief
     *     Projection of node pairs to the their term contributed to the sum in stress computation.
     *
     */
    struct node_stress
    {

        /**
         * @brief
         *     Non-throwing default constructor.
         *
         * @warning
         *     A default-constructed `node_stress` projection must never be called.
         *
         */
        node_stress() noexcept = default;

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
         * @param nodesep
         *     desired node separation
         *
         */
        node_stress(const ogdf::GraphAttributes& attrs,
                    const ogdf_node_array_2d<double>& matrix,
                    const double nodesep) noexcept
            : _attrs{&attrs}, _matrix{&matrix}, _nodesep{nodesep}
        {
        }

        /**
         * @brief
         *     Returns the stress term contributed by two nodes.
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

        /** @brief Referenced layout.  */
        const ogdf::GraphAttributes* _attrs{};

        /** @brief Referenced shortest path matrix.  */
        const ogdf_node_array_2d<double>* _matrix{};

        /** @brief Desired node separation.  */
        double _nodesep{};

    };  // struct node_stress

    /**
     * @brief
     *     Presents the range of terms contributing to the stress.
     *
     * The referenced layout and shortest path matrix must remain valid (and not be moved) while this range is used.
     *
     */
    class pairwise_stress final
    {
    public:

        /** @brief The exposed iterator type. */
        using iterator = node_pair_iterator<double, threshold_node_pair_predicate<double>, node_stress>;

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
         * @param nodesep
         *     desired node separation
         *
         * @param infinity
         *     value that is larger than the longest shortest path in the graph
         *
         */
        pairwise_stress(const ogdf::GraphAttributes& attrs,
                        const ogdf_node_array_2d<double>& matrix,
                        const double nodesep,
                        const double infinity) noexcept
            : _attrs{&attrs}, _matrix{&matrix}, _nodesep{nodesep}, _infty{infinity}
        {
        }

        /**
         * @brief
         *     Returns an iterator to the first term.
         *
         * @returns
         *     iterator to first term
         *
         */
        iterator begin() const noexcept
        {
            const auto pred = threshold_node_pair_predicate<double>{*_matrix, _infty};
            const auto proj = node_stress{*_attrs, *_matrix, _nodesep};
            return iterator{_attrs->constGraph(), pred, proj};
        }

        /**
         * @brief
         *     Returns an iterator after the last term.
         *
         * @returns
         *     iterator after last term
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

        /** @brief Desired node separation.  */
        double _nodesep{};

        /** @brief Value larger than the longest shortest path (between connected nodes) in the graph.  */
        double _infty{};

    };  // class pairwise_stress

}  // namespace msc

#define MSC_INCLUDED_FROM_STRESS_HXX
#include "stress.txx"
#undef MSC_INCLUDED_FROM_STRESS_HXX

#endif  // !defined(MSC_STRESS_HXX)
