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

#include <cstddef>
#include <stdexcept>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "data_analysis.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "rdf.hxx"

#define PROGRAM_NAME "rdf-local"

namespace /*anonymous*/
{

    double get_longest_path(const msc::ogdf_node_array_2d<double>& matrix, const ogdf::Graph& graph) noexcept
    {
        auto longest = 0.0;
        for (auto v1 = graph.firstNode(); v1 != nullptr; v1 = v1->succ()) {
            for (auto v2 = v1->succ(); v2 != nullptr; v2 = v2->succ()) {
                const auto v1tov2 = matrix[v1][v2];
                // OGDF inserts nonsensical large but finite values into the matrix for unreachable nodes.
                if ((v1tov2 <= graph.numberOfNodes()) && (v1tov2 > longest)) {
                    longest = v1tov2;
                }
            }
        }
        return longest;
    }

    struct application final
    {
        msc::cli_parameters_property_local parameters{};
        void operator()() const;
    };

    msc::json_object basic_info()
    {
        auto info = msc::json_object{};
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object
    do_local_rdf_for_vicinity(const msc::cli_parameters_property& params,
                              const msc::local_pairwise_distances& distances,
                              const int counter)
    {
        auto info = msc::json_object{};
        auto data = msc::json_array{};
        auto entropies = msc::initialize_entropies();
        auto analyzer = msc::data_analyzer{params.kernel};
        for (std::size_t i = 0; i < params.iterations(); ++i) {
            auto subinfo = msc::json_object{};
            analyzer.set_width(msc::get_item(params.width, i));
            analyzer.set_bins(msc::get_item(params.bins, i));
            analyzer.set_points(params.points);
            analyzer.set_output(msc::expand_filename(params.output, counter, i));
            if (analyzer.analyze_oknodo(distances, info, subinfo)) {
                msc::append_entropy(entropies, subinfo, "bincount");
                data.push_back(std::move(subinfo));
            }
        }
        if (data.empty()) {
            throw std::runtime_error{"Not enough data for a statistical analysis"};
        }
        msc::assign_entropy_regression(entropies, info);
        info["vicinity"] = msc::json_real{distances.limit()};
        info["data"] = std::move(data);
        return info;
    }

    struct file_name_nullifier final
    {
        template <typename JsonT>
        std::void_t<typename JsonT::primitive_json_tag_type> operator()(JsonT&) const
        {
        }

        void operator()(msc::json_array& arr) const
        {
            for (auto&& item : arr) {
                std::visit(*this, item);
            }
        }

        void operator()(msc::json_object& obj) const
        {
            for (auto&& item : obj) {
                std::visit(*this, item.second);
                if (item.first == "filename") {
                    assert(std::holds_alternative<msc::json_text>(item.second) ||
                           std::holds_alternative<msc::json_null>(item.second));
                    item.second = msc::json_null{};
                }
            }
        }

    };  // struct file_name_nullifier

    msc::json_object make_global_info(msc::json_object prototype)
    {
        file_name_nullifier{}(prototype);
        prototype["vicinity"] = msc::json_null{};
        return prototype;
    }

    msc::json_object make_local_info(msc::json_object prototype, const double vicinity = NAN)
    {
        prototype["vicinity"] = msc::json_real{vicinity};
        return prototype;
    }

    void application::operator()() const
    {
        const auto [graph, attrs] = msc::load_layout(this->parameters.input);
        const auto matrix = msc::get_pairwise_shortest_paths(*graph);
        const auto longestpath = get_longest_path(*matrix, *graph);
        auto distances = msc::local_pairwise_distances{*attrs, *matrix, NAN};
        auto sequence = msc::json_array{};
        if (this->parameters.vicinity.empty()) {
            for (auto vicinity = 1.0; true; vicinity *= 2.0) {
                distances.set_limit(vicinity);
                sequence.push_back(do_local_rdf_for_vicinity(this->parameters, distances, sequence.size()));
                if (vicinity >= longestpath) { break; }
            }
        } else {
            auto global = msc::json_object{};
            for (const auto vicinity : this->parameters.vicinity) {
                if ((vicinity > longestpath) && !global.empty()) {
                    sequence.push_back(make_local_info(global, vicinity));
                    continue;
                }
                distances.set_limit(vicinity);
                sequence.push_back(do_local_rdf_for_vicinity(this->parameters, distances, sequence.size()));
                if ((vicinity > longestpath) && global.empty()) {
                    global = make_global_info(std::get<msc::json_object>(sequence.back()));
                }
            }
        }
        auto info = basic_info();
        info["data"] = std::move(sequence);
        info["diameter"] = msc::json_real{longestpath};
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back(
        "Computes the local radial distribution function (RDF) for a graph layout.  Local means that only pairs of"
        " nodes will be considered for which the shortest path does not exceed a given vicinity."
    );
    app.help.push_back(msc::helptext_file_name_expansion());
    return app(argc, argv);
}
