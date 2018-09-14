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
#include <config.h>
#endif

#include "testaux/envguard.hxx"

#include <errno.h>
#include <stdlib.h>

#include <cassert>
#include <cstdio>
#include <system_error>
#include <utility>

namespace /*anonymous*/
{

    const char* do_getenv([[maybe_unused]] const std::string& name, std::error_code& ec) noexcept
    {
        using namespace std;
#if HAVE_POSIX_GETENV
        ec.clear();
        return getenv(name.c_str());
#else
        ec.assign(ENOSYS, std::generic_category());
        return nullptr;
#endif
    }

    const char* do_getenv([[maybe_unused]] const std::string& name)
    {
        auto ec = std::error_code{};
        const auto result = do_getenv(name, ec);
        return !ec ? result : throw std::system_error{ec, "getenv"};
    }

    bool do_setenv([[maybe_unused]] const std::string& name,
                   [[maybe_unused]] const std::string& value,
                   std::error_code& ec) noexcept
    {
        using namespace std;
#if HAVE_POSIX_SETENV
        if (setenv(name.c_str(), value.c_str(), true) == 0) {
            ec.clear();
            return true;
        } else {
            ec.assign(errno, std::system_category());
            return false;
        }
#else
        ec.assign(ENOSYS, std::generic_category());
        return false;
#endif
    }

    void do_setenv(const std::string& name, const std::string& value)
    {
        auto ec = std::error_code{};
        if (!do_setenv(name, value, ec)) {
            throw std::system_error{ec, "setenv"};
        }
    }

    bool do_unsetenv([[maybe_unused]] const std::string& name, std::error_code& ec) noexcept
    {
        using namespace std;
#if HAVE_POSIX_UNSETENV
        if (unsetenv(name.c_str()) == 0) {
            ec.clear();
            return true;
        } else {
            ec.assign(errno, std::system_category());
            return false;
        }
#else
        ec.assign(ENOSYS, std::generic_category());
        return false;
#endif
    }

    void do_unsetenv(const std::string& name)
    {
        auto ec = std::error_code{};
        if (!do_unsetenv(name, ec)) {
            throw std::system_error{ec, "unsetenv"};
        }
    }

    std::optional<std::string> make_optional_string(const char* s)
    {
        if (s == nullptr) {
            return std::nullopt;
        } else {
            return std::string{s};
        }
    }

}  // namespace /*anonymous*/

namespace msc::test
{

    envguard::envguard(std::string name)
    {
        assert(name.find('\0') == std::string::npos);
        _name = std::move(name);
        _previous = make_optional_string(do_getenv(_name));
    }

    envguard::~envguard()
    {
        auto ec = std::error_code{};
        if (!(_previous ? do_setenv(_name, *_previous, ec) : do_unsetenv(_name, ec))) {
            std::fprintf(stderr, "error: %s: Cannot reset environment variable: %s\n",
                         _name.c_str(), ec.message().c_str());
        }
    }

    void envguard::set(const std::string& value)
    {
        assert(value.find('\0') == std::string::npos);
        do_setenv(_name, value);
    }

    void envguard::unset()
    {
        do_unsetenv(_name);
    }

    bool envguard::can_be_used() noexcept
    {
        return HAVE_POSIX_GETENV && HAVE_POSIX_SETENV && HAVE_POSIX_UNSETENV;
    }

}  // namespace msc::test
