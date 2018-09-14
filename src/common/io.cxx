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

#include "io.hxx"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/fileformats/GraphIO.h>

#include "file.hxx"
#include "histogram.hxx"
#include "iosupp.hxx"
#include "stochastic.hxx"
#include "strings.hxx"
#include "useful.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        std::string format_unsupported_message(const std::string_view filename,
                                               const fileformats format,
                                               const int mode,
                                               const bool layout)
        {
            const auto what = [mode]{
                switch (mode) {
                case 'I': return "read";
                case 'O': return "write";
                default:  return "handle";
                }
            }();
            const auto with = layout ? "layouts" : "graphs";
            return concat(filename, ": Sorry, cannot ", what, " ", with, " in'", name(format), "' format");
        }

    }  // namespace /*anonymous*/

    unsupported_format::unsupported_format(const std::string_view filename,
                                           const fileformats format,
                                           const int mode,
                                           const bool layout)
        : std::runtime_error{format_unsupported_message(filename, format, mode, layout)}
    {
    }

    degenerated_layout::degenerated_layout(const std::string_view filename)
        : std::runtime_error{concat(filename, ": Degenerated layout")}
    {
    }

    namespace /*anonymous*/
    {

        constexpr int ios_success =  0;
        constexpr int ios_badform = -1;
        constexpr int ios_notsupp = -2;
        constexpr int ios_ioerror = +1;

        std::unique_ptr<ogdf::Graph>
        read_graph_from_stream(std::istream& istr,
                               const fileformats format,
                               const std::string_view filename = "/dev/stdin")
        {
            auto graph = std::make_unique<ogdf::Graph>();
            auto status = ios_badform;
            switch (format) {
            case fileformats::bench:         status = !ogdf::GraphIO::readGML          (*graph, istr); break;
            case fileformats::chaco:         status = !ogdf::GraphIO::readChaco        (*graph, istr); break;
            case fileformats::dl:            status = !ogdf::GraphIO::readDL           (*graph, istr); break;
            case fileformats::dmf:           status = ios_notsupp;                                     break;
            case fileformats::dot:           status = !ogdf::GraphIO::readDOT          (*graph, istr); break;
            case fileformats::gd_challenge:  status = ios_notsupp;                                     break;
            case fileformats::gdf:           status = !ogdf::GraphIO::readGDF          (*graph, istr); break;
            case fileformats::gexf:          status = !ogdf::GraphIO::readGEXF         (*graph, istr); break;
            case fileformats::gml:           status = !ogdf::GraphIO::readGML          (*graph, istr); break;
            case fileformats::graph6:        status = !ogdf::GraphIO::readGraph6       (*graph, istr); break;
            case fileformats::graphml:       status = !ogdf::GraphIO::readGraphML      (*graph, istr); break;
            case fileformats::leda:          status = !ogdf::GraphIO::readLEDA         (*graph, istr); break;
            case fileformats::matrix_market: status = !ogdf::GraphIO::readMatrixMarket (*graph, istr); break;
            case fileformats::pla:           status = ios_notsupp;                                     break;
            case fileformats::pm_diss_graph: status = !ogdf::GraphIO::readPMDissGraph  (*graph, istr); break;
            case fileformats::rome:          status = !ogdf::GraphIO::readRome         (*graph, istr); break;
            case fileformats::rudy:          status = ios_notsupp;                                     break;
            case fileformats::stp:           status = !ogdf::GraphIO::readSTP          (*graph, istr); break;
            case fileformats::tlp:           status = !ogdf::GraphIO::readTLP          (*graph, istr); break;
            case fileformats::ygraph:        status = !ogdf::GraphIO::readYGraph       (*graph, istr); break;
            }
            switch (status) {
            case ios_success: return graph;
            case ios_badform: reject_invalid_enumeration(format, "graph file format");
            case ios_notsupp: throw unsupported_format{filename, format, 'I', false};
            default:          report_io_error(filename, "Cannot read graph data");
            }
        }

        std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
        read_layout_from_stream(std::istream& istr,
                                const fileformats format,
                                const std::string_view filename = "/dev/stdin")
        {
            auto graph = std::make_unique<ogdf::Graph>();
            auto attrs = std::make_unique<ogdf::GraphAttributes>(*graph);
            auto status = ios_badform;
            switch (format) {
            case fileformats::bench:         status = !ogdf::GraphIO::readGML     (*attrs, *graph, istr); break;
            case fileformats::chaco:         status = ios_notsupp;                                        break;
            case fileformats::dl:            status = ios_notsupp;                                        break;
            case fileformats::dmf:           status = ios_notsupp;                                        break;
            case fileformats::dot:           status = !ogdf::GraphIO::readDOT     (*attrs, *graph, istr); break;
            case fileformats::gd_challenge:  status = ios_notsupp;                                        break;
            case fileformats::gdf:           status = !ogdf::GraphIO::readGDF     (*attrs, *graph, istr); break;
            case fileformats::gexf:          status = ios_notsupp;                                        break;
            case fileformats::gml:           status = !ogdf::GraphIO::readGML     (*attrs, *graph, istr); break;
            case fileformats::graph6:        status = ios_notsupp;                                        break;
            case fileformats::graphml:       status = !ogdf::GraphIO::readGraphML (*attrs, *graph, istr); break;
            case fileformats::leda:          status = ios_notsupp;                                        break;
            case fileformats::matrix_market: status = ios_notsupp;                                        break;
            case fileformats::pla:           status = ios_notsupp;                                        break;
            case fileformats::pm_diss_graph: status = ios_notsupp;                                        break;
            case fileformats::rome:          status = ios_notsupp;                                        break;
            case fileformats::rudy:          status = ios_notsupp;                                        break;
            case fileformats::stp:           status = ios_notsupp;                                        break;
            case fileformats::tlp:           status = !ogdf::GraphIO::readTLP     (*attrs, *graph, istr); break;
            case fileformats::ygraph:        status = ios_notsupp;                                        break;
            }
            switch (status) {
            case ios_success: return {std::move(graph), std::move(attrs)};
            case ios_badform: reject_invalid_enumeration(format, "layout file format");
            case ios_notsupp: throw unsupported_format{filename, format, 'I', true};
            default:          report_io_error(filename, "Cannot read layout data");
            }
        }

        void write_graph_to_stream(const ogdf::Graph& graph,
                                   std::ostream& ostr,
                                   const fileformats format,
                                   const std::string_view filename = "/dev/stdout")
        {
            auto status = ios_badform;
            switch (format) {
            case fileformats::bench:         status = !ogdf::GraphIO::writeGML          (graph, ostr); break;
            case fileformats::chaco:         status = !ogdf::GraphIO::writeChaco        (graph, ostr); break;
            case fileformats::dl:            status = !ogdf::GraphIO::writeDL           (graph, ostr); break;
            case fileformats::dmf:           status = ios_notsupp;                                     break;
            case fileformats::dot:           status = !ogdf::GraphIO::writeDOT          (graph, ostr); break;
            case fileformats::gd_challenge:  status = ios_notsupp;                                     break;
            case fileformats::gdf:           status = !ogdf::GraphIO::writeGDF          (graph, ostr); break;
            case fileformats::gexf:          status = !ogdf::GraphIO::writeGEXF         (graph, ostr); break;
            case fileformats::gml:           status = !ogdf::GraphIO::writeGML          (graph, ostr); break;
            case fileformats::graph6:        status = !ogdf::GraphIO::writeGraph6       (graph, ostr); break;
            case fileformats::graphml:       status = !ogdf::GraphIO::writeGraphML      (graph, ostr); break;
            case fileformats::leda:          status = !ogdf::GraphIO::writeLEDA         (graph, ostr); break;
            case fileformats::matrix_market: status = ios_notsupp;                                     break;
            case fileformats::pla:           status = ios_notsupp;                                     break;
            case fileformats::pm_diss_graph: status = !ogdf::GraphIO::writePMDissGraph  (graph, ostr); break;
            case fileformats::rome:          status = !ogdf::GraphIO::writeRome         (graph, ostr); break;
            case fileformats::rudy:          status = ios_notsupp;                                     break;
            case fileformats::stp:           status = ios_notsupp;                                     break;
            case fileformats::tlp:           status = !ogdf::GraphIO::writeTLP          (graph, ostr); break;
            case fileformats::ygraph:        status = ios_notsupp;                                     break;
            }
            switch (status) {
            case ios_badform: reject_invalid_enumeration(format, "graph file format");
            case ios_notsupp: throw unsupported_format{filename, format, 'O', false};
            case ios_success: if (ostr.flush().good()) return; [[fallthrough]];
            default:          report_io_error(filename, "Cannot write graph data");
            }
        }

        void write_layout_to_stream(const ogdf::GraphAttributes& attrs,
                                    std::ostream& ostr,
                                    const fileformats format,
                                    const std::string_view filename = "/dev/stdout")
        {
            auto status = ios_badform;
            switch (format) {
            case fileformats::bench:         status = !ogdf::GraphIO::writeGML     (attrs, ostr); break;
            case fileformats::chaco:         status = ios_notsupp;                                break;
            case fileformats::dl:            status = ios_notsupp;                                break;
            case fileformats::dmf:           status = ios_notsupp;                                break;
            case fileformats::dot:           status = !ogdf::GraphIO::writeDOT     (attrs, ostr); break;
            case fileformats::gd_challenge:  status = ios_notsupp;                                break;
            case fileformats::gdf:           status = !ogdf::GraphIO::writeGDF     (attrs, ostr); break;
            case fileformats::gexf:          status = ios_notsupp;                                break;
            case fileformats::gml:           status = !ogdf::GraphIO::writeGML     (attrs, ostr); break;
            case fileformats::graph6:        status = ios_notsupp;                                break;
            case fileformats::graphml:       status = !ogdf::GraphIO::writeGraphML (attrs, ostr); break;
            case fileformats::leda:          status = ios_notsupp;                                break;
            case fileformats::matrix_market: status = ios_notsupp;                                break;
            case fileformats::pla:           status = ios_notsupp;                                break;
            case fileformats::pm_diss_graph: status = ios_notsupp;                                break;
            case fileformats::rome:          status = ios_notsupp;                                break;
            case fileformats::rudy:          status = ios_notsupp;                                break;
            case fileformats::stp:           status = ios_notsupp;                                break;
            case fileformats::tlp:           status = !ogdf::GraphIO::writeTLP     (attrs, ostr); break;
            case fileformats::ygraph:        status = ios_notsupp;                                break;
            }
            switch (status) {
            case ios_badform: reject_invalid_enumeration(format, "layout file format");
            case ios_notsupp: throw unsupported_format{filename, format, 'O', true};
            case ios_success: if (ostr.flush().good()) return; [[fallthrough]];
            default:          report_io_error(filename, "Cannot write layout data");
            }
        }

        bool is_degenerated_layout(const ogdf::GraphAttributes& attrs) noexcept
        {
            auto tally = 0;
            for (const auto v : attrs.constGraph().nodes) {
                const auto x = attrs.x(v);
                const auto y = attrs.y(v);
                if (!std::isfinite(x) || !std::isfinite(y)) {
                    return true;
                }
                if ((x != 0.0) || (y != 0.0)) {
                    tally += 1;
                }
            }
            return (tally == 0) && (attrs.constGraph().numberOfNodes() > 1);
        }

    }  // namespace /*anonymous*/

    std::unique_ptr<ogdf::Graph>
    import_graph(const input_file& src, const fileformats form)
    {
        auto stream = boost::iostreams::filtering_istream{};
        const auto name = prepare_stream(stream, src);
        return read_graph_from_stream(stream, form, name);
    }

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    import_layout(const input_file& src, const fileformats form)
    {
        auto stream = boost::iostreams::filtering_istream{};
        const auto name = prepare_stream(stream, src);
        auto result = read_layout_from_stream(stream, form, name);
        if (is_degenerated_layout(*result.second)) {
            throw degenerated_layout{name};
        }
        return result;
    }

    void export_graph(const ogdf::Graph& graph, const output_file& dst, const fileformats form)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_graph_to_stream(graph, stream, form, name);
    }

    void export_layout(const ogdf::GraphAttributes& attrs, const output_file& dst, const fileformats form)
    {
        if (is_degenerated_layout(attrs)) {
            throw std::invalid_argument{"Cowardly refusing to save a degenerated layout"};
        }
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_layout_to_stream(attrs, stream, form, name);
    }

    std::unique_ptr<ogdf::Graph> load_graph(const input_file& src)
    {
        auto stream = boost::iostreams::filtering_istream{};
        const auto name = prepare_stream(stream, src);
        return read_graph_from_stream(stream, internal_file_format, name);
    }

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>> load_layout(const input_file& src)
    {
        auto stream = boost::iostreams::filtering_istream{};
        const auto name = prepare_stream(stream, src);
        auto result = read_layout_from_stream(stream, internal_file_format, name);
        if (is_degenerated_layout(*result.second)) {
            throw degenerated_layout{name};
        }
        return result;
    }

    void store_graph(const ogdf::Graph& graph, const output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_graph_to_stream(graph, stream, internal_file_format, name);
    }

    void store_layout(const ogdf::GraphAttributes& attrs, const output_file& dst)
    {
        if (is_degenerated_layout(attrs)) {
            throw std::invalid_argument{"Cowardly refusing to save a degenerated layout"};
        }
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_layout_to_stream(attrs, stream, internal_file_format, name);
    }

    namespace /*anonymous*/
    {

        void write_events(const std::vector<double>& data,
                          const stochastic_summary& summary,
                          std::ostream& ostr,
                          const std::string_view name)
        {
            constexpr auto digits = std::numeric_limits<double>::max_digits10;
            constexpr auto width = 26;
            ostr << std::setprecision(digits) << std::scientific
                 << "# Number of events:       " << std::setw(width) << summary.count << "\n"
                 << "# Minimum:                " << std::setw(width) << summary.min   << "\n"
                 << "# Maximum                 " << std::setw(width) << summary.max   << "\n"
                 << "# Arithmetic mean:        " << std::setw(width) << summary.mean  << "\n"
                 << "# Root mean square:       " << std::setw(width) << summary.rms   << "\n"
                 << std::endl;
            for (const auto event : data) {
                ostr << std::setw(width) << event << '\n';
            }
            if (!ostr.flush().good()) {
                report_io_error(name, "Cannot write event data");
            }
        }

        void write_frequencies(const histogram& histo, std::ostream& ostr, const std::string_view name)
        {
            constexpr auto digits = std::numeric_limits<double>::max_digits10;
            constexpr auto width = 26;
            ostr << std::setprecision(digits) << std::scientific
                 << "# Number of events:       " << std::setw(width) << histo.size()     << "\n"
                 << "# Bin count:              " << std::setw(width) << histo.bincount() << "\n"
                 << "# Minimum:                " << std::setw(width) << histo.min()      << "\n"
                 << "# Maximum:                " << std::setw(width) << histo.max()      << "\n"
                 << "# Arithmetic mean:        " << std::setw(width) << histo.mean()     << "\n"
                 << "# Root mean square:       " << std::setw(width) << histo.rms()      << "\n"
                 << "# Entropy:                " << std::setw(width) << histo.entropy()  << "\n"
                 << std::endl;
            for (std::size_t idx = 0; idx < histo.bincount(); ++idx) {
                ostr << std::setw(width) << histo.center(idx) << std::setw(width) << histo.frequency(idx) << '\n';
            }
            if (!ostr.flush().good()) {
                report_io_error(name, "Cannot write frequency data");
            }
        }

        void write_density(const std::vector<std::pair<double, double>>& density,
                           const stochastic_summary& summary,
                           std::ostream& ostr, const std::string_view filename)
        {
            constexpr auto digits = std::numeric_limits<double>::max_digits10;
            constexpr auto width = 26;
            ostr << std::setprecision(digits) << std::scientific
                 << "# Number of events:       " << std::setw(width) << summary.count    << "\n"
                 << "# Minimum:                " << std::setw(width) << summary.min      << "\n"
                 << "# Maximum:                " << std::setw(width) << summary.max      << "\n"
                 << "# Arithmetic mean:        " << std::setw(width) << summary.mean     << "\n"
                 << "# Root mean square:       " << std::setw(width) << summary.rms      << "\n"
                 << "# Density step count:     " << std::setw(width) << density.size()   << "\n"
                 << std::endl;
            for (const auto [x, y] : density) {
                ostr << std::setw(width) << x << std::setw(width) << y << '\n';
            }
            if (!ostr.flush().good()) {
                report_io_error(filename, "Cannot write density data");
            }
        }

    }  // namespace /*anonymous*/

    void write_events(const std::vector<double>& data,
                      const stochastic_summary& summary,
                      const output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_events(data, summary, stream, name);
    }

    void write_frequencies(const histogram& histo, const output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_frequencies(histo, stream, name);
    }

    void write_density(const std::vector<std::pair<double, double>>& density,
                       const stochastic_summary& summary,
                       const output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        write_density(density, summary, stream, name);
    }

}  // namespace msc
