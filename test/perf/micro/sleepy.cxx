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

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <thread>

#include "benchmark.hxx"

#define PROGRAM_NAME "sleepy"

namespace /*anonymous*/
{

    void benchmark()
    {
        const auto duration = std::chrono::microseconds{1};
        std::this_thread::sleep_for(duration);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    try {
        const auto t0 = msc::benchmark::clock_type::now();
        auto setup = msc::benchmark::benchmark_setup(
            PROGRAM_NAME,
            "Pseudo micro benchmark that artificially delays for 1 microsecond"
        );
        if (!setup.process(argc, argv)) {
            return EXIT_SUCCESS;
        }
        auto constr = setup.get_constraints();
        if (constr.timeout.count() > 0) {
            constr.timeout -= msc::benchmark::duration_type{msc::benchmark::clock_type::now() - t0};
        }
        const auto result = msc::benchmark::run_benchmark(constr, benchmark);
        msc::benchmark::print_result(result);
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << PROGRAM_NAME << ": error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
