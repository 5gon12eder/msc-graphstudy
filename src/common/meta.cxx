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

#include "meta.hxx"

#include "file.hxx"
#include "iosupp.hxx"
#include "json.hxx"

namespace msc
{

    void print_meta(const json_object& info, const output_file& dst)
    {
        auto stream = boost::iostreams::filtering_ostream{};
        const auto name = prepare_stream(stream, dst);
        if (!(stream << info << std::endl)) {
            report_io_error(name, "Cannot write JSON meta data data");
        }
    }

}  // namespace msc
