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
#include <exception>
#include <iostream>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "benchmark.hxx"
#include "enums/compressions.hxx"
#include "file.hxx"
#include "io.hxx"
#include "testaux/cube.hxx"
#include "testaux/tempfile.hxx"

#define PROGRAM_NAME "store"

namespace /*anonymous*/
{

    std::pair<std::unique_ptr<ogdf::Graph>, std::unique_ptr<ogdf::GraphAttributes>>
    make_test_data(const int n, const int m, const bool layout)
    {
        if (!layout) {
            return {msc::test::make_test_graph(n, m), nullptr};
        } else {
            return msc::test::make_test_layout(n, m);
        }
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    try {
        const auto t0 = msc::benchmark::clock_type::now();
        auto setup = msc::benchmark::benchmark_setup(
            PROGRAM_NAME,
            "I/O benchmark for storing graph and layout data"
        );
        setup.add_cmd_arg("nodes", "number of nodes");
        setup.add_cmd_arg("edges", "number of edges");
        setup.add_cmd_flag("layout", "store layout data as well");
        setup.add_cmd("compress", "specify compression algorithm", "none");
        if (!setup.process(argc, argv)) {
            return EXIT_SUCCESS;
        }
        const auto n = static_cast<int>(setup.get_cmd_arg("nodes"));
        const auto m = static_cast<int>(setup.get_cmd_arg("edges"));
        const auto layout = setup.get_cmd_flag("layout");
        const auto comp = msc::value_of_compressions(setup.get_cmd("compress"));
        const auto size = n + m;
        const auto temp = msc::test::tempfile{};
        std::clog << PROGRAM_NAME << ": Using temporary file: " << temp.filename() << "\n";
        const auto [graph, attrs] = make_test_data(n, m, layout);
        auto constr = setup.get_constraints();
        if (constr.timeout.count() > 0) {
            constr.timeout -= msc::benchmark::duration_type{msc::benchmark::clock_type::now() - t0};
        }
        const auto file = msc::output_file::from_filename(temp.filename(), comp);
        const auto absres = !layout
            ? msc::benchmark::run_benchmark(constr, msc::store_graph,  *graph, file)
            : msc::benchmark::run_benchmark(constr, msc::store_layout, *attrs, file);
        const auto relres = msc::benchmark::result{absres.mean / size, absres.stdev / size, absres.n};
        msc::benchmark::print_result(relres);
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << PROGRAM_NAME << ": error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
