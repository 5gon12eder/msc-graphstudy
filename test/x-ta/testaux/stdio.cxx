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

#include "testaux/stdio.hxx"

namespace msc::test
{

    capture_stdio::capture_stdio(const std::string& input) :
        _oldbufin{std::cin.rdbuf()},
        _oldbufout{std::cout.rdbuf()},
        _oldbuferr{std::cerr.rdbuf()},
        _tempin{input}
    {
        std::cin.rdbuf(_tempin.rdbuf());
        std::cout.rdbuf(_tempout.rdbuf());
        std::cerr.rdbuf(_temperr.rdbuf());
    }

    capture_stdio::~capture_stdio() noexcept
    {
        std::cin.rdbuf(_oldbufin);
        std::cout.rdbuf(_oldbufout);
        std::cerr.rdbuf(_oldbuferr);
    }

    std::string capture_stdio::get_stdout() const
    {
        return _tempout.str();
    }

    std::string capture_stdio::get_stderr() const
    {
        return _temperr.str();
    }

}  // namespace msc::test
