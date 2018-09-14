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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

#include <boost/iostreams/filtering_stream.hpp>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/graphics.h>
#include <ogdf/fileformats/GraphIO.h>

#include "cli.hxx"
#include "file.hxx"
#include "io.hxx"
#include "iosupp.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "strings.hxx"

#define PROGRAM_NAME "picture"

namespace /*anonymous*/
{

    void reshape_layout(ogdf::GraphAttributes& attrs)
    {
        attrs.translateToNonNeg();
        const auto bbox = msc::get_bounding_box_size(attrs);
        const auto xscale = 1000.0 / bbox.x();
        const auto yscale = 1000.0 / bbox.y();
        const auto scale = std::min(xscale, yscale);
        attrs.scale(scale, false);
        for (const auto v : attrs.constGraph().nodes) {
            attrs.shape(v) = ogdf::Shape::Ellipse;
            attrs.width(v) = 5.0;
            attrs.height(v) = 5.0;
        }
    }

    void add_principial_axes(ogdf::Graph& graph,
                             ogdf::GraphAttributes& attrs,
                             const msc::point2d major,
                             const msc::point2d minor,
                             const ogdf::Color color)
    {
        const auto v0 = graph.newNode();
        const auto v1 = graph.newNode();
        const auto v2 = graph.newNode();
        const auto v3 = graph.newNode();
        const auto e1 = graph.newEdge(v0, v1);
        const auto e2 = graph.newEdge(v2, v3);
        attrs.x(v0) = -major.x(); attrs.y(v0) = -major.y();
        attrs.x(v1) = +major.x(); attrs.y(v1) = +major.y();
        attrs.x(v2) = -minor.x(); attrs.y(v2) = -minor.y();
        attrs.x(v3) = +minor.x(); attrs.y(v3) = +minor.y();
        for (const auto v : {v0, v1, v2, v3}) {
            attrs.fillColor(v) = color;
            attrs.strokeColor(v) = color;
        }
        for (const auto e : {e1, e2}) {
            attrs.strokeColor(e) = color;
        }
    }

    void colorize_layout(ogdf::GraphAttributes& attrs, const ogdf::Color node_color, const ogdf::Color edge_color)
    {
        attrs.addAttributes(ogdf::GraphAttributes::nodeStyle | ogdf::GraphAttributes::edgeStyle);
        for (const auto v : attrs.constGraph().nodes) {
            attrs.fillColor(v) = node_color;
            attrs.strokeColor(v) = node_color;
        }
        for (const auto e : attrs.constGraph().edges) {
            attrs.strokeColor(e) = edge_color;
        }
    }

    void write_picture_svg(const ogdf::GraphAttributes& attrs, const msc::output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = msc::prepare_stream(stream, dst);
        if (!ogdf::GraphIO::drawSVG(attrs, stream) || !stream.flush().good()) {
            msc::report_io_error(name, "Cannot write SVG data");
        }
    }

    void write_picture_tikz(const ogdf::GraphAttributes& attrs,
                            const msc::point2d& major,
                            const msc::point2d& minor,
                            const msc::output_file& dst)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        const auto scale = 1.0 / std::max(bbox.x(), bbox.y());
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = msc::prepare_stream(stream, dst);
        stream << std::setprecision(10) << std::fixed;
        stream << "% bounding box size: " << bbox << "\n";
        stream << "\\iftikzgraphpreamble\n";
        stream << "\\def\\aspectratio{" << bbox.y() / bbox.x() << "}\n";
        stream << "\\else\n";
        for (const auto v : attrs.constGraph().nodes) {
            const auto [x, y] = scale * msc::get_coords(attrs, v);
            stream << "\\node[vertex] (v" << v->index() << ") at (" << x << ", " << y << ") {};\n";
        }
        for (const auto e : attrs.constGraph().edges) {
            stream << "\\draw[edge] (v" << e->source()->index() << ") -- (v" << e->target()->index() << ");\n";
        }
        if (major && minor) {
            const auto [x1, y1] = scale * major;
            const auto [x2, y2] = scale * minor;
            stream << "\\draw[princomp1st]"
                   << " (0, 0) -- (" << -x1 << ", " << -y1 << ") -- (0, 0) -- (" << -x2 << ", " << -y2 << ") --"
                   << " (0, 0) -- (" << +x1 << ", " << +y1 << ") -- (0, 0) -- (" << +x2 << ", " << +y2 << ") --"
                   << " cycle\n";
        } else if (major) {
            const auto [x, y] = scale * major;
            stream << "\\draw[princomp1st] (" << -x << ", " << -y << ") -- (" << +x << ", " << +y << ");\n";
        } else if (minor) {
            const auto [x, y] = scale * minor;
            stream << "\\draw[princomp2nd] (" << -x << ", " << -y << ") -- (" << +x << ", " << +y << ");\n";
        }
        stream << "\\fi\n";
        if (!stream.flush().good()) {
            msc::report_io_error(name, "Cannot write SVG data");
        }
        stream << "\n";
    }

    struct cli_parameters
    {
        msc::input_file input{"-"};
        msc::output_file output{"-"};
        msc::output_file meta{};
        msc::point2d major{};
        msc::point2d minor{};
        ogdf::Color node_color{};
        ogdf::Color edge_color{};
        ogdf::Color axis_color{};
        bool tikz{};
    };

    struct application final
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const ogdf::GraphAttributes& attrs, const ogdf::Color nc, const ogdf::Color ec)
    {
        auto info = msc::json_object{};
        info["nodes"] = msc::json_diff{attrs.constGraph().numberOfNodes()};
        info["edges"] = msc::json_diff{attrs.constGraph().numberOfEdges()};
        info["node-color"] = nc.toString();
        info["edge-color"] = ec.toString();
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        auto [graph, attrs] = msc::load_layout(this->parameters.input);
        if (this->parameters.tikz) {
            write_picture_tikz(*attrs, this->parameters.major, this->parameters.minor, this->parameters.output);
        } else {
            colorize_layout(*attrs, this->parameters.node_color, this->parameters.edge_color);
            if (const auto [major, minor] = std::tie(this->parameters.major, this->parameters.minor); major && minor) {
                add_principial_axes(*graph, *attrs, major, minor, this->parameters.axis_color);
            }
            reshape_layout(*attrs);
            write_picture_svg(*attrs, this->parameters.output);
        }
        const auto info = get_info(*attrs, this->parameters.node_color, this->parameters.edge_color);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Draws a layout as an SVG picture.");
    return app(argc, argv);
}
