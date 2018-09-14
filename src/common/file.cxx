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

#include "file.hxx"

#include "iosupp.hxx"
#include "strings.hxx"

namespace msc
{

    file::file() noexcept
    {
    }

    file::file(const std::string_view spec)
    {
        if (!spec.empty()) {
            const auto [destspec, algospec] = split_filename(spec);
            if (is_nullio(destspec)) {
                _terminal = terminals::null;
            } else if (is_stdio(destspec)) {
                _terminal = terminals::stdio;
            } else if (const auto fd = is_fdno(destspec)) {
                _terminal = terminals::descriptor;
                _descriptor = check_descriptor(fd.value());
            } else {
                _terminal = terminals::file;
                _filename = concat(check_filename(destspec));
            }
            const auto compression = algospec.empty() ? compressions::automatic : value_of_compressions(algospec);
            _compression = (compression == compressions::automatic) ? guess_compression(_filename) : compression;
        }
    }

    file file::from_filename(const std::string_view filename, const compressions compression)
    {
        check_filename(filename);
        auto self = file{};
        self._terminal = terminals::file;
        self._compression = (compression == compressions::automatic) ? guess_compression(filename) : compression;
        self._filename.assign(filename.data(), filename.size());
        return self;
    }

    file file::from_descriptor(const int descriptor, const compressions compression)
    {
        check_descriptor(descriptor);
        auto self = file{};
        self._terminal = terminals::descriptor;
        self._compression = (compression == compressions::automatic) ? compressions::none : compression;
        self._descriptor = descriptor;
        return self;
    }

    file file::from_null(const compressions compression)
    {
        auto self = file{};
        self._terminal = terminals::null;
        self._compression = (compression == compressions::automatic) ? compressions::none : compression;
        return self;
    }

    file file::from_stdio(const compressions compression)
    {
        auto self = file{};
        self._terminal = terminals::stdio;
        self._compression = (compression == compressions::automatic) ? compressions::none : compression;
        return self;
    }

    void file::assign_from_spec(const std::string_view spec)
    {
        *this = file{spec};
    }

    int file::mode() const noexcept
    {
        return 0;
    }

    int input_file::mode() const noexcept
    {
        return 'I';
    }

    int output_file::mode() const noexcept
    {
        return 'O';
    }

}  // namespace msc
