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
 * @file meta.hxx
 *
 * @brief
 *     Functionality for writing meta data.
 *
 * All tools should support the `--meta` or `-m` option in their CLIs in order to emit meta data in JSON format.  This
 * component provides utilities to handle this in a coherent way.
 *
 */

#ifndef MSC_META_HXX
#define MSC_META_HXX

namespace msc
{

    struct json_object;
    struct output_file;

    /**
     * @brief
     *     Serializes and writes the meta data in `info` to `dest`.
     *
     * @param info
     *     JSON object to serialize and write
     *
     * @param dest
     *     destination to write to
     *
     * @throws std::system_error
     *     if there was an error writing to the file
     *
     */
    void print_meta(const json_object& info, const output_file& dest);

}  // namespace msc

#endif  // !defined(MSC_META_HXX)
