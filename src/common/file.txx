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

#ifndef MSC_INCLUDED_FROM_FILE_HXX
#  error "Never `#include <file.txx>` directly, `#include <file.hxx>` instead"
#endif

#include <utility>

namespace msc
{

    inline terminals file::terminal() const noexcept
    {
        return _terminal;
    }

    inline compressions file::compression() const noexcept
    {
        return _compression;
    }

    inline const std::string& file::filename() const noexcept
    {
        return _filename;
    }

    inline int file::descriptor() const noexcept
    {
        return _descriptor;
    }

    template <terminals Term, typename... ArgTs>
    void file::assign(ArgTs&&... args)
    {
        if constexpr (Term == terminals{}) {
            *this = file{std::forward<ArgTs>(args)...};
        } else if constexpr (Term == terminals::null) {
            *this = file::from_null(std::forward<ArgTs>(args)...);
        } else if constexpr (Term == terminals::stdio) {
            *this = file::from_stdio(std::forward<ArgTs>(args)...);
        } else if constexpr (Term == terminals::file) {
            *this = file::from_filename(std::forward<ArgTs>(args)...);
        } else if constexpr (Term == terminals::descriptor) {
            *this = file::from_descriptor(std::forward<ArgTs>(args)...);
        } else {
            static_assert(Term == terminals{}, "A valid terminal type is required");
        }
    }

    inline bool operator==(const file& lhs, const file& rhs) noexcept
    {
        return (lhs.terminal() == rhs.terminal())
            && (lhs.compression() == rhs.compression())
            && (lhs.filename() == rhs.filename())
            && (lhs.descriptor() == rhs.descriptor());
    }

    inline bool operator!=(const file& lhs, const file& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    inline input_file::input_file(file src) : file{std::move(src)}
    {
    }

    inline output_file::output_file(file dst) : file{std::move(dst)}
    {
    }

}  // namespace msc
