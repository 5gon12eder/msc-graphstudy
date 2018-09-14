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

#include <memory>
#include <random>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "random.hxx"

#define PROGRAM_NAME "perturb"

namespace /*anonymous*/
{

    template <typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    worsen(EngineT& engine, const ogdf::GraphAttributes& attrs, const double rate)
    {
        // NB: Instantiating `std::normal_distribution` with `sigma == 0.0` is UB so we multiply with `rate` later.
        auto distribution = std::normal_distribution<double>{0.0, msc::default_node_distance};
        auto worse = std::make_unique<ogdf::GraphAttributes>(attrs.constGraph());
        for (const auto v : attrs.constGraph().nodes) {
            const auto orig = msc::point2d{attrs.x(v), attrs.y(v)};
            const auto diff = msc::make_random_point<double, 2>(engine, distribution);
            const auto pert = orig + rate * diff;
            worse->x(v) = pert.x();
            worse->y(v) = pert.y();
        }
        msc::normalize_layout(*worse);
        return worse;
    }

    struct application final
    {
        msc::cli_parameters_worsening parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const std::string& seed)
    {
        auto info = msc::json_object{};
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object get_subinfo(const ogdf::GraphAttributes& attrs, const double rate, const std::string& filename)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["filename"] = filename;
        info["layout"] = msc::layout_fingerprint(attrs);
        info["rate"] = msc::json_real{rate};
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        auto [graph, attrs] = msc::load_layout(this->parameters.input);
        auto info = get_info(seed);
        auto data = msc::json_array{};
        for (const auto rate : this->parameters.rate) {
            const auto dest = this->parameters.expand_filename(rate);
            const auto worse = worsen(rndeng, *attrs, rate);
            msc::store_layout(*worse, dest);
            data.push_back(get_subinfo(*worse, rate, dest.filename()));
        }
        info["data"] = std::move(data);
        msc::print_meta(info, this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Worsens a given layout by randomly flipping edges.");
    return app(argc, argv);
}
