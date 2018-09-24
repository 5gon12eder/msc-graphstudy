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

/**
 * @file io.hxx
 *
 * @brief
 *     Utility functions for consistent and convenient I/O of graph, layout and property data.
 *
 */

#ifndef MSC_IO_HXX
#define MSC_IO_HXX

#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "enums/fileformats.hxx"
#include "ogdf_fwd.hxx"

namespace msc
{

    // Forward-declarations
    class histogram;
    struct input_file;
    struct output_file;
    struct stochastic_summary;

    /** @brief Preferred graph and layout file format.  */
    constexpr fileformats internal_file_format = fileformats::graphml;

    /**
     * @brief
     *     Exception that indicates that an I/O operation failed because the requested file format is not supported.
     *
     * The reason might be that the file format is not implemented or (for layouts) that the format has no support for
     * storing coordinates of nodes.
     *
     */
    struct unsupported_format : std::runtime_error
    {
        /**
         * @brief
         *     Creates a new exception object.
         *
         * @param filename
         *     name of the file involved
         *
         * @param
         *     format that is not supported
         *
         * @param mode
         *     `'I'`, `'O'` or `'I' + 'O'` depending on whether reading, writing or both is not supported
         *
         * @param layout
         *     whether (only) layouts are not supported in this format
         *
         */
        unsupported_format(std::string_view filename, fileformats format, int mode, bool layout);

    };  // struct unsupported_format

    /**
     * @brief
     *     Exception that indicates that an I/O operation failed because the input file had graph but no layout data.
     *
     */
    struct degenerated_layout : std::runtime_error
    {

        /**
         * @brief
         *     Creates a new exception bemoaning that the given file name contained a degenerated layout.
         *
         * @param filename
         *     name of the file involved
         *
         */
        degenerated_layout(std::string_view filename);

    };  // struct degenerated_layout

    /**
     * @brief
     *     Loads a graph from a file in the specified format.
     *
     * @param src
     *     file to read from
     *
     * @param format
     *     graph file format
     *
     * @returns
     *     the graph
     *
     * @throws std::exception
     *     if no graph can be read from the given file
     *
     */
    std::unique_ptr<ogdf::Graph>
    import_graph(const input_file& src, fileformats format);

    /**
     * @brief
     *     Loads a layout from a file in the specified format.
     *
     * @param src
     *     file to read from
     *
     * @param format
     *     layout file format
     *
     * @returns
     *     a pair with the graph and layout
     *
     * @throws std::exception
     *     if no layout can be read from the given file
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    import_layout(const input_file& src, fileformats format);

    /**
     * @brief
     *     Loads a layout from a file in the specified format if present; otherwise just the graph.
     *
     * @param src
     *     file to read from
     *
     * @param format
     *     layout file format
     *
     * @returns
     *     a pair with the graph and layout (the latter may be null)
     *
     * @throws std::exception
     *     if no layout or graph can be read from the given file
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    import_layout_or_graph(const input_file& src, fileformats format);

    /**
     * @brief
     *     Stores a graph in a file using the default format.
     *
     * @param graph
     *     graph to store
     *
     * @param dst
     *     file to write to
     *
     * @param format
     *     graph file format
     *
     * @throws std::exception
     *     if the graph cannot be written to the given file
     *
     */
    void export_graph(const ogdf::Graph& graph, const output_file& dst, fileformats format);

    /**
     * @brief
     *     Stores a layout in a file using the default format.
     *
     * @param attrs
     *     layout to store
     *
     * @param dst
     *     file to write to
     *
     * @param format
     *     layout file format
     *
     * @throws std::exception
     *     if the layout cannot be written to the given file
     *
     */
    void export_layout(const ogdf::GraphAttributes& attrs, const output_file& dst, fileformats format);

    /**
     * @brief
     *     Loads a graph from a file in the internal format.
     *
     * @param src
     *     file to read from
     *
     * @returns
     *     the graph
     *
     * @throws std::exception
     *     if no graph can be read from the given file
     *
     */
    std::unique_ptr<ogdf::Graph>
    load_graph(const input_file& src);

    /**
     * @brief
     *     Loads a layout from a file in the internal format.
     *
     * @param src
     *     file to read from
     *
     * @returns
     *     a pair with the graph and layout
     *
     * @throws std::exception
     *     if no layout can be read from the given file
     *
     */
    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    load_layout(const input_file& src);

    /**
     * @brief
     *     Stores a graph in a file using the internal format.
     *
     * @param graph
     *     graph to store
     *
     * @param dst
     *     file to write to
     *
     * @throws std::exception
     *     if the graph cannot be written to the given file
     *
     */
    void store_graph(const ogdf::Graph& graph, const output_file& dst);

    /**
     * @brief
     *     Stores a layout in a file using the internal format.
     *
     * @param attrs
     *     layout to store
     *
     * @param dst
     *     file to write to
     *
     * @throws std::exception
     *     if the layout cannot be written to the given file
     *
     */
    void store_layout(const ogdf::GraphAttributes& attrs, const output_file& dst);

    /**
     * @brief
     *     Writes event data to a text file.
     *
     * The format is such that it can be processed by Gnuplot and similar tools.
     *
     * @param data
     *     event data to write
     *
     * @param summary
     *     stochastic summary of the event data
     *
     * @param dst
     *     file to write to
     *
     * @throws std::exception
     *     if the data cannot be written to the given file
     *
     */
    void write_events(const std::vector<double>& data, const stochastic_summary& summary, const output_file& dst);

    /**
     * @brief
     *     Writes frequency data to a text file.
     *
     * The format is such that it can be processed by Gnuplot and similar tools.
     *
     * @param histo
     *     binned frequency data to write
     *
     * @param dst
     *     file to write to
     *
     * @throws std::exception
     *     if the data cannot be written to the given file
     *
     */
    void write_frequencies(const histogram& histo, const output_file& dst);

    /**
     * @brief
     *     Writes density data to a text file.
     *
     * The format is such that it can be processed by Gnuplot and similar tools.
     *
     * @param density
     *     array of (<var>x</var>, &rho;(<var>x</var>)) points describing the normalized density
     *
     * @param summary
     *     stochastic summary of the event data
     *
     * @param dst
     *     file to write to
     *
     * @throws std::exception
     *     if the data cannot be written to the given file
     *
     */
    void write_density(const std::vector<std::pair<double, double>>& density,
                       const stochastic_summary& summary,
                       const output_file& dst);

}  // namespace msc

#endif  // !defined(MSC_IO_HXX)
