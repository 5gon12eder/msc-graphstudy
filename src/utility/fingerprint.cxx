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
#include <string>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "ogdf_fix.hxx"

#define PROGRAM_NAME "fingerprint"

namespace /*anonymous*/
{

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file meta{};
        bool layout{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::Graph& graph, const std::string_view filename)
    {
        auto info = msc::json_object{};
        info["filename"] = msc::json_text{filename};
        info["graph"] = msc::graph_fingerprint(graph);
        info["nodes"] = msc::json_diff{graph.numberOfNodes()};
        info["edges"] = msc::json_diff{graph.numberOfEdges()};
        return info;
    }

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const std::string_view filename)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = get_info(attrs.constGraph(), filename);
        info["layout"] = msc::layout_fingerprint(attrs);
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void application::operator()() const
    {
        if (this->parameters.layout) {
            const auto [graph, attrs] = msc::load_layout(this->parameters.input);
            const auto info = get_info(*attrs, this->parameters.input.filename());
            msc::print_meta(info, this->parameters.meta);
            (void) graph.get();
        } else {
            const auto graph = msc::load_graph(this->parameters.input);
            auto info = get_info(*graph, this->parameters.input.filename());
            msc::print_meta(info, this->parameters.meta);
        }
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Reports fingerprint and other useful information.");
    return app(argc, argv);
}
