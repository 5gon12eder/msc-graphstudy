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
 * @file pairwise.hxx
 *
 * @brief
 *     Common facilities for making statistics on pairs of nodes.
 *
 * @warning
 *     This header actually includes headers from the OGDF rather than just forward-declaring some types.
 *
 */

#ifndef MSC_PAIRWISE_HXX
#define MSC_PAIRWISE_HXX

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <ogdf/basic/Graph.h>

namespace msc
{

    /** @brief Convenient type alias for a node property. */
    template <typename T>
    using ogdf_node_array_1d = ogdf::NodeArray<T>;

    /** @brief Convenient type alias for a pairwise node property. */
    template <typename T>
    using ogdf_node_array_2d = ogdf::NodeArray<ogdf::NodeArray<T>>;

    /** @brief Convenient type alias for a pair of node pointers. */
    using node_pair = std::pair<ogdf::node, ogdf::node>;

    /**
     * @brief
     *     Computes all pairwise shortest paths in a graph.
     *
     * @param graph
     *     graph to operate on
     *
     * @returns
     *     pairwise shortest path matrix
     *
     */
    std::unique_ptr<ogdf_node_array_2d<double>> get_pairwise_shortest_paths(const ogdf::Graph& graph);

    /**
     * @brief
     *     A predicate that filters all pairs of nodes.
     *
     * This is the prototypical filter type to use with `node_pair_iterator`s.  The `constexpr` qualifiers are not
     * required for compliance.  Predicates may also have stronger preconditions.
     *
     */
    struct tautology_node_pair_predicate /*non-final*/
    {

        /** @brief Non-throwing default constructor. */
        constexpr tautology_node_pair_predicate() noexcept = default;

        /**
         * @brief
         *     Returns the predicate for the nodes `v1` and `v2`.
         *
         * @param v1
         *     first node
         *
         * @param v2
         *     second node
         *
         * @returns
         *     `true`
         *
         */
        constexpr bool operator()([[maybe_unused]] const ogdf::node v1,
                                  [[maybe_unused]] const ogdf::node v2) const noexcept
        {
            return true;
        }

    };  // struct constant_node_pair_filter

    /**
     * @brief
     *     An identity projection.
     *
     * This is the prototypical projection type to use with `node_pair_iterator`s.  The `constexpr` qualifiers are not
     * required for compliance.  Projections may also have stronger preconditions.
     *
     */
    struct identity_node_pair_projection /*non-final*/
    {

        /** @brief Non-throwing default constructor. */
        constexpr identity_node_pair_projection() noexcept = default;

        /**
         * @brief
         *     Returns the predicate of the nodes `v1` and `v2`.
         *
         * @param v1
         *     first node
         *
         * @param v2
         *     second node
         *
         * @returns
         *     a pair containing the two nodes
         *
         */
        constexpr node_pair operator()(const ogdf::node v1, const ogdf::node v2) const noexcept
        {
            return {v1, v2};
        }

    };  // struct identity_node_pair_projection

    /**
     * @brief
     *     A generic iterator that iterates over a projection of filtered pairs of nodes in a layout.
     *
     * Let <var>G</var> = (<var>V</var>, <var>E</var>) be an undirected graph `graph`.  Furthermore, let
     * `pred` : <var>V</var> &times; <var>V</var> &rarr; {0, 1} be a predicate for pairs of nodes and `proj` :
     * <var>V</var> &times; <var>V</var> &rarr; <var>T</var> be a projection that maps pairs of nodes to instances of a
     * type `T`.
     *
     * The following pseudo C++ code ...
     *
     *     const auto first = node_pair_iterator<T, decltype(pred)>{graph, pred};
     *     const auto last  = node_pair_iterator<T, decltype(pred)>{};
     *     const auto items = std::vector<T>(first, last);
     *
     * ... will roughly have the same semantics as the following hypothetical code in Python ...
     *
     *     items = [proj(v1, v2) for (v1, v2) in itertools.combinations(graph.nodes, 2) if pred(v1, v2)]
     *
     * ... which, as a fellow functional programmer, you'd obviously write in this much more succinct way.
     *
     *     items = list(map(proj, filter(pred, itertools.combinations(graph.nodes, 2))))
     *
     * Due to (possibly misguided) performance considerations, this class enforces an aggressive application of
     * `noexcept` qualifiers on functions that might not be able to gurantee that they always succeed or that have
     * preconditions, willingly accepting that `std::terminate` might be called.
     *
     * The iterator is evaluated eagerly.  That is, once incremented, dereferencing it multiple times is cheap.
     *
     * @tparam ValueT
     *     type stored in the iterator (Requires nothrow default constructible and nothrow copyable and nothrow
     *     constructible.  However, a default-constructed `ValueT` need not have a meaningful value and will never be
     *     the result of dereferencing the iterator.)
     *
     * @tparam PredT
     *     type of the predicate (Requires nothrow default constructible and nothrow copyable.  However, a
     *     default-constructed `PredT` will never be called.)
     *
     * @tparam ProjT
     *     type of the predicate (Requires nothrow default constructible and nothrow copyable.  However, a
     *     default-constructed `ProjT` will never be called.)
     *
     */
    template
    <
        typename ValueT = node_pair,
        typename PredT = tautology_node_pair_predicate,
        typename ProjT = identity_node_pair_projection
    >
    class node_pair_iterator final : private PredT, private ProjT
    {

        // We don't assert on the "nothrow" part because many standard library types do not apply `noexcept` as
        // agressively as we might.

        static_assert(
            std::is_default_constructible_v<ValueT>,
            "The value type of a node pair iterator must be nothrow default constructible. "
            " Such a default-constructed value need not be in a usable state, though."
        );

        static_assert(
            std::is_default_constructible_v<PredT>,
            "The predicate type of a node pair iterator must be nothrow default constructible. "
            " Such a default-constructed policy need not be in a usable state, though."
        );

        static_assert(
            std::is_default_constructible_v<ProjT>,
            "The projection type of a node pair iterator must be nothrow default constructible. "
            " Such a default-constructed policy need not be in a usable state, though."
        );

        static_assert(
            std::is_copy_constructible_v<ValueT> && std::is_copy_assignable_v<ValueT>,
            "The value type of a node pair iterator must be nothrow copyable."
        );

        static_assert(
            std::is_copy_constructible_v<PredT> && std::is_copy_assignable_v<PredT>,
            "The predicate type of a node pair iterator must be nothrow copyable."
        );

        static_assert(
            std::is_copy_constructible_v<ProjT> && std::is_copy_assignable_v<ProjT>,
            "The projection type of a node pair iterator must be nothrow copyable."
        );

        static_assert(
            std::is_nothrow_invocable_r_v<bool, const PredT&, ogdf::node, ogdf::node>,
            "The predicate of a node pair iterator must be callable with with two node pointers and the result of the"
            " call -- which must not throw -- must be convertible to a boolean value."
        );

        static_assert(
            std::is_nothrow_invocable_r_v<ValueT, const ProjT&, ogdf::node, ogdf::node>,
            "The projection of a node pair iterator must be callable with with two node pointers and and the result of"
            " the call -- which must not throw -- must be convertible to the value type."
        );

    public:

        /** @brief Iterator catagory of the iterator. */
        using iterator_category = std::forward_iterator_tag;

        /** @brief Value type of the iterator. */
        using value_type = std::add_const_t<ValueT>;

        /** @brief Difference type of the iterator. */
        using difference_type = std::ptrdiff_t;

        /** @brief Pointer type of the iterator. */
        using pointer = std::add_pointer_t<value_type>;

        /** @brief Reference type of the iterator. */
        using reference = std::add_lvalue_reference_t<value_type>;

        /**
         * @brief
         *     Constructs a special past-the-end iterator that can be compared with any other iterator of this type.
         *
         */
        node_pair_iterator() noexcept = default;

        /**
         * @brief
         *     Constructs an iterator that points to the first node pair of the given layout that passes the policy.
         *
         * @param graph
         *     the graph to iterate over
         *
         * @param pred
         *     predicate (if stateful)
         *
         * @param proj
         *     projection (if stateful)
         *
         */
        explicit node_pair_iterator(const ogdf::Graph& graph, PredT pred = PredT{}, ProjT proj = ProjT{}) noexcept :
            PredT{std::move(pred)}, ProjT{std::move(proj)}, _graph{&graph}
        {
            _v1 = _graph->firstNode();
            _v2 = (_v1 == nullptr) ? nullptr : _v1->succ();
            if (_good()) {
                while (!_pred()(_v1, _v2) && _advance_once()) continue;
                if (_good()) _value = _proj()(_v1, _v2);
            }
        }

        /**
         * @brief
         *     Advances the iterator and returns a reference to the advanced iterator (pre-increment).
         *
         * The behavior of this function is undefined on a past-the-end iterator.
         *
         * @returns
         *     reference to the advanced iterator
         *
         */
        node_pair_iterator& operator++() noexcept
        {
            _advance();
            return *this;
        }

        /**
         * @brief
         *     Advances the iterator and returns a copy of the iterator before it was advanced (post-increment).
         *
         * The behavior of this function is undefined on a past-the-end iterator.
         *
         * Note that `node_pair_iterator`s are relatively large and therefore expensive to copy so this function is best
         * avoided in favor of the pre-increment operator.
         *
         * @returns
         *     reference to the advanced iterator
         *
         */
        node_pair_iterator operator++(int) noexcept
        {
            const auto before = *this;
            _advance();
            return before;
        }

        /**
         * @brief
         *     Dereferences the iterator and returns a reference to the current value.
         *
         * The behavior of this function is undefined on a past-the-end iterator.
         *
         * @returns
         *     reference to the current value
         *
         */
        reference operator*() const noexcept
        {
            assert(_good());
            return _value;
        }

        /**
         * @brief
         *     Dereferences the iterator and returns a pointer to the current value.
         *
         * The behavior of this function is undefined on a past-the-end iterator.
         *
         * @returns
         *     pointer to the current value
         *
         */
        pointer operator->() const noexcept
        {
            assert(_good());
            return std::addressof(_value);
        }

        /**
         * @brief
         *     Tests whether the iterator is /not/ a past-the-end iterator.
         *
         * @returns
         *     whether the iterator may be derefnerenced or advanced
         *
         */
        explicit operator bool() const noexcept
        {
            return _good();
        }

        /**
         * @brief
         *     Compares two iterators for equality.
         *
         * Equality of `node_pair_iterator`s is defined as follows.
         *
         * 1. Two past-the-end iterators always compare equal.
         * 2. A past-the-end iterator never compared equal with another iterator that is not a past-the-end iterator.
         * 3. Two non-past-the-end iterators compare equal if and only if they both refer to the same pair of nodes.
         *
         * The behavior of comparing two iterators from different layouts is undefined.  The only exception to this is a
         * default-constructed (past-the-end) iterator that may be compared with any other iterator and compares equal
         * to any other iterator if and only if the other iterator is a past-the-end iterator, too.
         *
         * @returns
         *     whether the two iterators are equal
         *
         */
        friend bool operator==(const node_pair_iterator& lhs, const node_pair_iterator& rhs) noexcept
        {
            assert((lhs._graph == nullptr) || (rhs._graph == nullptr) || (lhs._graph == rhs._graph));
            if (!lhs._good()) return !rhs._good();
            if (!rhs._good()) return !lhs._good();
            return (lhs._v1 == rhs._v1) && (lhs._v2 == rhs._v2);
        }

        /**
         * @brief
         *     Compares two iterators for inequality.
         *
         * Please refer to the documentation of the equality operator for the definition of equality and the
         * preconditions of this function.
         *
         * @returns
         *     whether the two iterators are unequal
         *
         */
        friend bool operator!=(const node_pair_iterator& lhs, const node_pair_iterator& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:

        const ogdf::Graph* _graph{nullptr};
        ogdf::node _v1{nullptr};
        ogdf::node _v2{nullptr};
        ValueT _value{};

        void _advance() noexcept
        {
            while (_advance_once() && !_pred()(_v1, _v2)) continue;
            if (_good()) _value = _proj()(_v1, _v2);
        }

        bool _advance_once() noexcept
        {
            if (_v1 == nullptr)       return false;  // TBD: Can this check be elided?
            if (_v2 == nullptr)       return false;  // TBD: Can this check be elided?
            if ((_v2 = _v2->succ()))  return true;
            if (!(_v1 = _v1->succ())) return false;
            if (!(_v2 = _v1->succ())) return false;
            return true;
        }

        bool _good() const noexcept
        {
            return (_v1 != nullptr) && (_v2 != nullptr);
        }

        const PredT& _pred() const noexcept
        {
            return *this;
        }

        const ProjT& _proj() const noexcept
        {
            return *this;
        }

    };  // class node_pair_iterator

    /**
     * @brief
     *     A meta-predicate that filters pairs of nodes based on a threshold and a pre-computed table.
     *
     * Let <var>G</var> = (<var>V</var>, <var>E</var>) be a graph, <var>f</var> : <var>V</var> &times; <var>V</var>
     * &rarr; <b>R</b> be a real projection of node pairs and <var>x</var> &isin; <b>R</b> be an arbitrary constant.
     * Then a pair of nodes (<var>v</var><sub>1</sub>, <var>v</var><sub>2</sub>) &isin; <var>V</var> &times;
     * <var>V</var> will satisfy this predicate if and only if <var>f</var>(<var>v</var><sub>1</sub>,
     * <var>v</var><sub>2</sub>) &le; <var>x</var> holds.
     *
     * @tparam T
     *     result type of the pre-computed function (must be nothrow default constructible and less-equal comparable)
     *
     */
    template <typename T>
    class threshold_node_pair_predicate /*non-final*/
    {
    public:

        /**
         * @brief
         *     Non-throwing default constructor.
         *
         * @warning
         *     A default-constructed predicate must never be called.
         *
         */
        threshold_node_pair_predicate() noexcept = default;

        /**
         * @brief
         *     Actually useful constructor.
         *
         * @param matrix
         *     pre-computed table of function values
         *
         * @param threshold
         *     largest value to let pass
         *
         */
        threshold_node_pair_predicate(const ogdf_node_array_2d<T>& matrix, const T threshold) noexcept
            : _matrix{&matrix}, _threshold{threshold}
        {
        }

        /**
         * @brief
         *     Tests whether pair of nodes `v1` and `v2` satisfy the predicate.
         *
         * The behavior is undefined if `v1` or `v2` are `nullptr` or do not belong to the graph for which the function
         * table was pre-computed.  Consequently, this function always invokes undefined behavior on a
         * default-constructed filter.
         *
         * @param v1
         *     first node
         *
         * @param v2
         *     second node
         *
         * @returns
         *     predicate for the node pair
         *
         */
        bool operator()(const ogdf::node v1, const ogdf::node v2) const noexcept
        {
            assert(_matrix != nullptr);
            assert(v1 != nullptr);
            assert(v2 != nullptr);
            return ((*_matrix)[v1][v2] <= _threshold);
        }

    private:

        /** @brief Pointer to pre-computed function table.  */
        const ogdf_node_array_2d<double>* _matrix{};

        /** @brief Largest value to accept.  */
        T _threshold{};

    };  // class threshold_node_pair_predicate

}  // namespace msc

#endif  // !defined(MSC_PAIRWISE_HXX)
