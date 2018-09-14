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

#define MSC_RUN_ALL_UNIT_TESTS_IN_MAIN

#include "pairwise.hxx"

#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "testaux/cube.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

    constexpr auto huge_distance = std::numeric_limits<float>::max();

    template <typename T>
    std::pair<T, T> make_unordered_pair(T a, T b)
    {
        const auto cmp = std::less<T>{};
        return cmp(a, b)
            ? std::make_pair(std::move(a), std::move(b))
            : std::make_pair(std::move(b), std::move(a));
    }

    MSC_AUTO_TEST_CASE(shortest_none)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
    }

    MSC_AUTO_TEST_CASE(shortest_singleton)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto matrix = msc::get_pairwise_shortest_paths(std::as_const(*graph));
        MSC_REQUIRE_EQ(0, (*matrix)[v1][v1]);
    }

    MSC_AUTO_TEST_CASE(shortest_pair_connected)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        graph->newEdge(v1, v2);
        const auto matrix = msc::get_pairwise_shortest_paths(std::as_const(*graph));
        MSC_REQUIRE_EQ(0, (*matrix)[v1][v1]);
        MSC_REQUIRE_EQ(1, (*matrix)[v1][v2]);
        MSC_REQUIRE_EQ(1, (*matrix)[v2][v1]);
        MSC_REQUIRE_EQ(0, (*matrix)[v2][v2]);
    }

    MSC_AUTO_TEST_CASE(shortest_pair_disconnected)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto matrix = msc::get_pairwise_shortest_paths(std::as_const(*graph));
        MSC_REQUIRE_EQ(0, (*matrix)[v1][v1]);
        MSC_REQUIRE_GE((*matrix)[v1][v2], huge_distance);
        MSC_REQUIRE_GE((*matrix)[v2][v1], huge_distance);
        MSC_REQUIRE_EQ(0, (*matrix)[v2][v2]);
    }

    // The graph looks like this:
    //
    //         1
    //         |
    //      +--2--+
    //      |     |
    //      3-----4
    //
    MSC_AUTO_TEST_CASE(shortest_thingy)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto v1 = graph->newNode();
        const auto v2 = graph->newNode();
        const auto v3 = graph->newNode();
        const auto v4 = graph->newNode();
        graph->newEdge(v1, v2);
        graph->newEdge(v2, v3);
        graph->newEdge(v2, v4);
        graph->newEdge(v3, v4);
        const auto matrix = msc::get_pairwise_shortest_paths(std::as_const(*graph));
        MSC_REQUIRE_EQ(0, (*matrix)[v1][v1]);
        MSC_REQUIRE_EQ(1, (*matrix)[v1][v2]);
        MSC_REQUIRE_EQ(2, (*matrix)[v1][v3]);
        MSC_REQUIRE_EQ(2, (*matrix)[v1][v4]);
        MSC_REQUIRE_EQ(1, (*matrix)[v2][v1]);
        MSC_REQUIRE_EQ(0, (*matrix)[v2][v2]);
        MSC_REQUIRE_EQ(1, (*matrix)[v2][v3]);
        MSC_REQUIRE_EQ(1, (*matrix)[v2][v4]);
        MSC_REQUIRE_EQ(2, (*matrix)[v3][v1]);
        MSC_REQUIRE_EQ(1, (*matrix)[v3][v2]);
        MSC_REQUIRE_EQ(0, (*matrix)[v3][v3]);
        MSC_REQUIRE_EQ(1, (*matrix)[v3][v4]);
        MSC_REQUIRE_EQ(2, (*matrix)[v4][v1]);
        MSC_REQUIRE_EQ(1, (*matrix)[v4][v2]);
        MSC_REQUIRE_EQ(1, (*matrix)[v4][v3]);
        MSC_REQUIRE_EQ(0, (*matrix)[v4][v4]);
    }

    MSC_AUTO_TEST_CASE(msc_tnpp)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto node1 = graph->newNode();
        const auto node2 = graph->newNode();
        const auto pred = msc::tautology_node_pair_predicate{};
        MSC_REQUIRE_EQ(true, pred(node1, node2));
    }

    MSC_AUTO_TEST_CASE(msc_inpp)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto node1 = graph->newNode();
        const auto node2 = graph->newNode();
        const auto proj = msc::identity_node_pair_projection{};
        MSC_REQUIRE_EQ(std::make_pair(node1, node2), proj(node1, node2));
    }

    MSC_AUTO_TEST_CASE(npi_default_empty)
    {
        const auto graph = std::make_unique<ogdf::Graph>();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        MSC_REQUIRE_EQ(first, last);
        MSC_REQUIRE_EQ(0, std::distance(first, last));
    }

    MSC_AUTO_TEST_CASE(npi_default_singleton)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        graph->newNode();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        MSC_REQUIRE_EQ(first, last);
        MSC_REQUIRE_EQ(0, std::distance(first, last));
    }

    MSC_AUTO_TEST_CASE(npi_default_pair)
    {
        auto graph = std::make_unique<ogdf::Graph>();
        const auto node1 = graph->newNode();
        const auto node2 = graph->newNode();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        MSC_REQUIRE_EQ(1, std::distance(first, last));
        MSC_REQUIRE_EQ(node1, first->first);
        MSC_REQUIRE_EQ(node2, first->second);
    }

    MSC_AUTO_TEST_CASE(npi_default_cube)
    {
        const auto graph = msc::test::make_cube_graph();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        const auto actual = std::vector<msc::node_pair>(first, last);
        const auto expected = [&graph](){
            auto vec = std::vector<msc::node_pair>{};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = v1->succ(); v2 != nullptr; v2 = v2->succ()) {
                    vec.emplace_back(v1, v2);
                }
            }
            return vec;
        }();
        MSC_REQUIRE_EQ(expected, actual);
    }

    MSC_AUTO_TEST_CASE(npi_traits)
    {
        using traits = std::iterator_traits<msc::node_pair_iterator<>>;
        static_assert(sizeof(traits::difference_type) > 0);
        static_assert(sizeof(traits::value_type) > 0);
        static_assert(sizeof(traits::pointer) > 0);
        static_assert(sizeof(traits::reference) > 0);
        static_assert(sizeof(traits::iterator_category) > 0);
    }

    MSC_AUTO_TEST_CASE(npi_boolctx)
    {
        const auto graph = msc::test::make_cube_graph();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        MSC_REQUIRE_EQ(true, bool(first));
        MSC_REQUIRE_EQ(false, bool(last));
        auto iter = first;
        while (iter != last) {
            MSC_REQUIRE_EQ(true, bool(iter++));
        }
        MSC_REQUIRE_EQ(false, bool(iter));
    }

    MSC_AUTO_TEST_CASE(npi_increment)
    {
        const auto graph = msc::test::make_cube_graph();
        auto iter = msc::node_pair_iterator<>{*graph};
        MSC_REQUIRE_EQ(iter, iter);
        MSC_REQUIRE_EQ(*iter, *iter);
        {
            const auto before = iter;
            MSC_REQUIRE(++iter != before);
        }
        {
            const auto before = iter;
            MSC_REQUIRE(*(++iter) != *before);
        }
        {
            const auto before = iter;
            MSC_REQUIRE(iter++ == before);
        }
        {
            const auto before = iter;
            MSC_REQUIRE(*(iter++) == *before);
        }
        MSC_REQUIRE_EQ(graph->firstNode(), iter->first);
    }

    MSC_AUTO_TEST_CASE(npi_multipass)
    {
        const auto graph = msc::test::make_cube_graph();
        const auto first = msc::node_pair_iterator<>{*graph};
        const auto last = msc::node_pair_iterator<>{};
        const auto firstpass = std::vector<msc::node_pair>(first, last);
        const auto secondpass = std::vector<msc::node_pair>(first, last);
        MSC_REQUIRE_EQ(12 + 12 + 4, firstpass.size());
        MSC_REQUIRE_EQ(12 + 12 + 4, secondpass.size());
        MSC_REQUIRE_EQ(firstpass, secondpass);
    }

    struct lookup_filter
    {
        using lookup_table_type = std::set<msc::node_pair>;
        lookup_filter() noexcept = default;
        lookup_filter(const lookup_table_type& lut) noexcept : _lut{&lut} { }
        bool operator()(const ogdf::node v1, const ogdf::node v2) const noexcept
        {
            return _lut->count(make_unordered_pair(v1, v2));
        }
    private:
        const lookup_table_type* _lut{};
    };

    std::set<msc::node_pair> edge_set(const ogdf::Graph& graph)
    {
        auto set = std::set<msc::node_pair>{};
        for (const auto edge : graph.edges) {
            set.insert(make_unordered_pair(edge->source(), edge->target()));
        }
        return set;
    }

    MSC_AUTO_TEST_CASE(npi_custom_predicate)
    {
        const auto graph = msc::test::make_cube_graph();
        const auto edges = edge_set(*graph);
        const auto first = msc::node_pair_iterator<msc::node_pair, lookup_filter>{*graph, lookup_filter{edges}};
        const auto last = msc::node_pair_iterator<msc::node_pair, lookup_filter>{};
        const auto actual = std::set<msc::node_pair>(first, last);
        MSC_REQUIRE_EQ(12, actual.size());
        MSC_REQUIRE_EQ(edges, actual);
    }

    struct remembers_everything
    {
        inline static std::vector<msc::node_pair> history{};

        remembers_everything() noexcept = default;

        std::nullptr_t operator()(const ogdf::node v1, const ogdf::node v2) const noexcept
        {
            history.emplace_back(v1, v2);
            return nullptr;
        }

        static auto guard()
        {
            const auto cleaner = [](auto* vp){ vp->clear(); };
            return std::unique_ptr<decltype(history), decltype(cleaner)>{&history, cleaner};
        }

    };

    MSC_AUTO_TEST_CASE(npi_custom_projection_all)
    {
        using iterator = msc::node_pair_iterator<
            std::nullptr_t,
            msc::tautology_node_pair_predicate,
            remembers_everything
        >;
        const auto guard = remembers_everything::guard();
        const auto graph = msc::test::make_cube_graph();
        const auto last = iterator{};
        MSC_REQUIRE_EQ(0, remembers_everything::history.size());
        const auto first = iterator{*graph};
        MSC_REQUIRE_EQ(1, remembers_everything::history.size());
        auto iter = msc::node_pair_iterator<>{*graph};
        for (auto it = first; it != last; ++it) {
            const auto [v1, v2] = remembers_everything::history.back();
            MSC_REQUIRE_EQ(v1, iter->first);
            MSC_REQUIRE_EQ(v2, iter->second);
            ++iter;
        }
        MSC_REQUIRE_EQ(28, remembers_everything::history.size());
    }

    struct oxymoron_node_pair_predicate
    {
        bool operator()(ogdf::node, ogdf::node) const noexcept { return false; }
    };

    MSC_AUTO_TEST_CASE(npi_custom_projection_none)
    {
        using iterator = msc::node_pair_iterator<
            std::nullptr_t,
            oxymoron_node_pair_predicate,
            remembers_everything
        >;
        const auto guard = remembers_everything::guard();
        const auto graph = msc::test::make_cube_graph();
        const auto first = iterator{*graph};
        const auto last = iterator{};
        for (auto it = first; it != last; ++it) {
            MSC_FAIL("This loop should not execute");
        }
        MSC_REQUIRE_EQ(0, remembers_everything::history.size());
    }

    MSC_AUTO_TEST_CASE(vicinity_npp_negative)
    {
        const double limits[] = {-1.0E-10, -1.0, -1.0E100, -HUGE_VAL, -INFINITY};
        const auto graph = msc::test::make_cube_graph();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        for (const auto limit : limits) {
            const auto lnpp = msc::threshold_node_pair_predicate<double>{*matrix, limit};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = graph->firstNode(); v2 != nullptr; v2 = v2->succ()) {
                    MSC_REQUIRE_EQ(false, lnpp(v1, v2));
                }
            }
        }
    }

    MSC_AUTO_TEST_CASE(vicinity_npp_0)
    {
        const double limits[] = {0.0, 0.5, 1.0 - 1.0E-10};
        const auto graph = msc::test::make_cube_graph();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        for (const auto limit : limits) {
            const auto lnpp = msc::threshold_node_pair_predicate<double>{*matrix, limit};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = graph->firstNode(); v2 != nullptr; v2 = v2->succ()) {
                    const auto issame = (v1 == v2);
                    const auto filter = lnpp(v1, v2);
                    MSC_REQUIRE_EQ(issame, filter);
                }
            }
        }
    }

    MSC_AUTO_TEST_CASE(vicinity_npp_1)
    {
        const double limits[] = {1.0, 1.5, 2.0 - 1.0E-10};
        const auto graph = msc::test::make_cube_graph();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto edges = edge_set(*graph);
        for (const auto limit : limits) {
            const auto lnpp = msc::threshold_node_pair_predicate<double>{*matrix, limit};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = graph->firstNode(); v2 != nullptr; v2 = v2->succ()) {
                    const auto isedge = (edges.find(make_unordered_pair(v1, v2)) != edges.cend());
                    const auto issame = (v1 == v2);
                    const auto filter = lnpp(v1, v2);
                    MSC_REQUIRE_IMPLIES(issame, filter);
                    MSC_REQUIRE_IMPLIES(isedge, filter);
                    MSC_REQUIRE_LE(int{filter}, int{isedge} + int{issame});
                }
            }
        }
    }

    MSC_AUTO_TEST_CASE(vicinity_npp_2)
    {
        const double limits[] = {2.0, 2.5, 3.0 - 1.0E-10};
        const auto graph = msc::test::make_cube_graph();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto edges = edge_set(*graph);
        for (const auto limit : limits) {
            auto tally = 0;
            const auto lnpp = msc::threshold_node_pair_predicate<double>{*matrix, limit};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = graph->firstNode(); v2 != nullptr; v2 = v2->succ()) {
                    if (!lnpp(v1, v2)) {
                        tally += 1;
                    }
                }
            }
            MSC_REQUIRE_EQ(8, tally);
        }
    }

    MSC_AUTO_TEST_CASE(vicinity_npp_3)
    {
        const double limits[] = {3.0, 3.0 + 1.0E-10, 1.0E100, HUGE_VAL, INFINITY};
        const auto graph = msc::test::make_cube_graph();
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto edges = edge_set(*graph);
        for (const auto limit : limits) {
            const auto lnpp = msc::threshold_node_pair_predicate<double>{*matrix, limit};
            for (auto v1 = graph->firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = graph->firstNode(); v2 != nullptr; v2 = v2->succ()) {
                    MSC_REQUIRE_EQ(true, lnpp(v1, v2));
                }
            }
        }
    }

}  // namespace /*anonymous*/
