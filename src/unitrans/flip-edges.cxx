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
#include "random.hxx"

#define PROGRAM_NAME "flip-edges"

namespace /*anonymous*/
{

    template <typename EngineT>
    std::unique_ptr<ogdf::GraphAttributes>
    worsen(EngineT /*by-value*/ engine, const ogdf::GraphAttributes& attrs, const double rate)
    {
        auto flipdist = std::uniform_real_distribution{};
        auto worse = std::make_unique<ogdf::GraphAttributes>(attrs.constGraph());
        for (const auto v : attrs.constGraph().nodes) {
            worse->x(v) = attrs.x(v);
            worse->y(v) = attrs.y(v);
        }
        for (const auto e : attrs.constGraph().edges) {
            if (flipdist(engine) < rate / 2.0) {
                const auto v1 = e->source();
                const auto v2 = e->target();
                worse->x(v1) = attrs.x(v2);
                worse->y(v1) = attrs.y(v2);
                worse->x(v2) = attrs.x(v1);
                worse->y(v2) = attrs.y(v1);
            }
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
