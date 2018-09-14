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

#include <cstdlib>
#include <memory>
#include <random>
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/basic.h>
#include <ogdf/layered/SugiyamaLayout.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "random.hxx"

#define PROGRAM_NAME "sugiyama"

namespace /*anonymous*/
{

    auto do_layout(const ogdf::Graph& graph) -> std::unique_ptr<ogdf::GraphAttributes>
    {
        auto attrs = std::make_unique<ogdf::GraphAttributes>(graph);
        attrs->directed() = false;
        auto layout = ogdf::SugiyamaLayout{};
        layout.call(*attrs);
        msc::normalize_layout(*attrs);
        return attrs;
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file meta{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const msc::output_file& dst, const std::string& seed)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        info["filename"] = msc::make_json(dst.filename());
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::default_random_engine{};
        const auto seed = msc::seed_random_engine(rndeng);
        std::srand(std::uniform_int_distribution<unsigned>{}(rndeng));
        ogdf::setSeed(std::uniform_int_distribution<int>{}(rndeng));
        const auto graph = msc::load_graph(this->parameters.input);
        const auto attrs = do_layout(*graph);
        msc::store_layout(*attrs, this->parameters.output);
        const auto info = get_info(*attrs, this->parameters.output, seed);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes a Sugiyama layout for the given graph.");
    return app(argc, argv);
}
