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
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "data_analysis.hxx"
#include "edge_length.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "edge-length"

namespace /*anonymous*/
{

    struct application final
    {
        msc::cli_parameters_property parameters{};
        void operator()() const;
    };

    msc::json_object basic_info()
    {
        auto info = msc::json_object{};
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        const auto [graph, attrs] = msc::load_layout(this->parameters.input);
        auto info = basic_info();
        auto subinfos = msc::json_array{};
        auto lengths = msc::get_all_edge_lengths(*attrs);
        auto analyzer = msc::data_analyzer{this->parameters.kernel};
        auto entropies = msc::initialize_entropies();
        for (std::size_t i = 0; i < this->parameters.iterations(); ++i) {
            auto subinfo = msc::json_object{};
            analyzer.set_width(msc::get_item(this->parameters.width, i));
            analyzer.set_bins(msc::get_item(this->parameters.bins, i));
            analyzer.set_points(this->parameters.points);
            analyzer.set_output(msc::expand_filename(this->parameters.output, i));
            analyzer.analyze(std::begin(lengths), std::end(lengths), info, subinfo);
            msc::append_entropy(entropies, subinfo, "bincount");
            subinfos.push_back(std::move(subinfo));
        }
        info["data"] = std::move(subinfos);
        msc::assign_entropy_regression(entropies, info);
        msc::print_meta(info, this->parameters.meta);
        (void) graph.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Computes the distribution of edge lengths.");
    app.help.push_back(msc::helptext_file_name_expansion());
    return app(argc, argv);
}
