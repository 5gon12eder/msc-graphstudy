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

#include <cmath>
#include <memory>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "math_constants.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"

#define PROGRAM_NAME "rotate"

namespace /*anonymous*/
{

    std::unique_ptr<ogdf::GraphAttributes> worsen(const ogdf::GraphAttributes& attrs, const double rate)
    {
        assert((0.0 <= rate) && (rate <= 1.0));
        const auto angle = 2.0 * M_PI * (1.0 - rate);  // clock-wise
        const auto rotate = [cos = std::cos(angle), sin = std::sin(angle)](const msc::point2d p)->msc::point2d{
            return {cos * p.x() - sin * p.y(), sin * p.x() + cos * p.y()};
        };
        auto worse = std::make_unique<ogdf::GraphAttributes>(attrs.constGraph());
        for (const auto v : attrs.constGraph().nodes) {
            const auto rot = rotate({attrs.x(v), attrs.y(v)});
            worse->x(v) = rot.x();
            worse->y(v) = rot.y();
        }
        msc::normalize_layout(*worse);
        return worse;
    }

    struct application final
    {
        msc::cli_parameters_worsening parameters{};
        void operator()() const;
    };

    msc::json_object get_info()
    {
        auto info = msc::json_object{};
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
        const auto [graph, attrs] = msc::load_layout(this->parameters.input);
        auto info = get_info();
        auto data = msc::json_array{};
        for (const auto rate : this->parameters.rate) {
            const auto dest = this->parameters.expand_filename(rate);
            const auto worse = worsen(*attrs, rate);
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
    app.help.push_back("Applies a clock-wise rotation between 0 (r = 0) and 360 (r = 1) degrees to a layout.");
    return app(argc, argv);
}
