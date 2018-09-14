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

#include <cassert>
#include <numeric>
#include <random>

#include <boost/iterator/transform_iterator.hpp>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "data_analysis.hxx"
#include "edge_length.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "point.hxx"
#include "princomp.hxx"
#include "random.hxx"

#define PROGRAM_NAME "princomp"

namespace /*anonymous*/
{

    struct cli_parameters : msc::cli_parameters_property
    {
        int component{0};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object basic_info(const std::string& seed)
    {
        auto info = msc::json_object{};
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    std::vector<msc::point2d> load_layout_as_point_cloud(const msc::input_file& src)
    {
        const auto [graph, attrs] = msc::load_layout(src);
        auto coords = std::vector<msc::point2d>{};
        coords.reserve(graph->numberOfNodes());
        for (const auto v : graph->nodes) {
            coords.emplace_back(attrs->x(v), attrs->y(v));
        }
        // Our internal layouts are normalized, which we shall rely on.  This is checked via an assertion because we'll
        // process an unnormalized layout just fine (without triggering UB) but the result will be wrong.
        assert(abs(std::accumulate(std::begin(coords), std::end(coords), msc::point2d{})) < 1.0E-6);
        return coords;
    }

    template <typename T, std::size_t N>
    msc::json_array point2json(const msc::point<T, N>& p)
    {
        auto coords = msc::json_array(N);
        for (std::size_t i = 0; i < N; ++i) {
            coords[i] = msc::json_real{p[i]};
        }
        return coords;
    }

    msc::point2d find_axis(const std::vector<msc::point2d>& coords, std::mt19937& engine, const int component)
    {
        using constant_1 = std::integral_constant<std::size_t, 1>;
        using constant_2 = std::integral_constant<std::size_t, 2>;
        switch (component) {
        case 1:
            return msc::find_primary_axes_nondestructive(coords, engine, constant_1{}).back();
        case 2:
            return msc::find_primary_axes_nondestructive(coords, engine, constant_2{}).back();
        default:
            throw std::invalid_argument{"Invalid component selection: " + std::to_string(component)};
        }
    }

    void application::operator()() const
    {
        auto engine = std::mt19937{};
        const auto seed = msc::seed_random_engine(engine);
        const auto coords = load_layout_as_point_cloud(this->parameters.input);
        const auto axis = find_axis(coords, engine, this->parameters.component);
        auto info = basic_info(seed);
        info["component"] = point2json(axis);
        auto subinfos = msc::json_array{};
        auto analyzer = msc::data_analyzer{this->parameters.kernel};
        auto entropies = msc::initialize_entropies();
        for (std::size_t i = 0; i < this->parameters.iterations(); ++i) {
            auto subinfo = msc::json_object{};
            analyzer.set_width(msc::get_item(this->parameters.width, i));
            analyzer.set_bins(msc::get_item(this->parameters.bins, i));
            analyzer.set_points(this->parameters.points);
            analyzer.set_output(msc::expand_filename(this->parameters.output, i));
            const auto getter = [axis](const auto& p){ return dot(axis, p); };
            const auto first = boost::make_transform_iterator(std::begin(coords), getter);
            const auto last = boost::make_transform_iterator(std::end(coords), getter);
            analyzer.analyze(first, last, info, subinfo);
            msc::append_entropy(entropies, subinfo, "bincount");
            subinfos.push_back(std::move(subinfo));
        }
        info["data"] = std::move(subinfos);
        msc::assign_entropy_regression(entropies, info);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes coordinate distribution along a principial axis.");
    app.help.push_back(
        "It is an error if neither the '--major' nor the '--minor' option (or their short versions '-1' and '-2')"
        " are passed.  In case more than one such option is passed, it is unspecified which one will take precedence."
    );
    app.help.push_back(msc::helptext_file_name_expansion());
    return app(argc, argv);
}
