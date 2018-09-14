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
#include <thread>

#include "cli.hxx"

#define PROGRAM_NAME "sleepy"

namespace /*anonymous*/
{

    struct cli_parameters { };

    struct application
    {
        cli_parameters parameters{};
        void operator()() const;
    };

    void application::operator()() const
    {
        const auto duration = std::chrono::milliseconds{10};
        std::this_thread::sleep_for(duration);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Sleeps for 10 milliseconds and then exits.");
    return app(argc, argv);
}
