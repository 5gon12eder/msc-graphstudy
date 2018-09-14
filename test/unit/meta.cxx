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

#define MSC_RUN_ALL_UNIT_TESTS_IN_MAIN

#include "meta.hxx"

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#if __has_include(<unistd.h>)
#  include <unistd.h>
#endif

#include "file.hxx"
#include "json.hxx"

#include "testaux/stdio.hxx"
#include "testaux/tempfile.hxx"
#include "unittest.hxx"

namespace /*anonymous*/
{

#if !HAVE_POSIX_FILENO && !defined(fileno)
    inline int fileno(...) noexcept { return -1; }
#endif

#if !HAVE_POSIX_WRITE && !defined(write)
    inline int write(...) noexcept { return -1; }
#endif

    template <typename T>
    std::string stringize(const T& obj, const std::string_view end = "\n")
    {
        auto oss = std::ostringstream{};
        oss << obj << end;
        return oss.str();
    }

    msc::json_object get_silly_info()
    {
        auto info = msc::json_object{};
        info["file"] = msc::json_text{ __FILE__ };
        info["line"] = msc::json_size{ __LINE__ };
        info["nothing"] = msc::json_null{};
        auto array = msc::json_array{};
        for (auto i = 1; i < 10; ++i) {
            array.push_back(msc::json_real{static_cast<double>(i)});
        }
        info["numbers"] = std::move(array);
        return info;
    }

    MSC_AUTO_TEST_CASE(meta_none)
    {
        using namespace std::string_view_literals;
        const auto guard = msc::test::capture_stdio{};
        const auto info = get_silly_info();
        msc::print_meta(info, msc::file{});
        MSC_REQUIRE_EQ(""sv, guard.get_stdout());
    }

    MSC_AUTO_TEST_CASE(meta_stdout)
    {
        const auto guard = msc::test::capture_stdio{};
        const auto info = get_silly_info();
        msc::print_meta(info, msc::file{"-"});
        // This test no longer works since we're bypassing std::cout and using STDOUT_FILENO directly.
        MSC_REQUIRE(guard.get_stdout().empty());
    }

    MSC_AUTO_TEST_CASE(meta_filename)
    {
        const auto tempfile = msc::test::tempfile{".json"};
        const auto info = get_silly_info();
        msc::print_meta(info, msc::file::from_filename(tempfile.filename()));
        MSC_REQUIRE_EQ(stringize(info), tempfile.read());
    }

    MSC_AUTO_TEST_CASE(meta_descriptor)
    {
        using namespace std;  // Because we don't know where non-standard functions will live...
        MSC_SKIP_UNLESS(HAVE_POSIX_WRITE && HAVE_POSIX_FILENO);
        const auto info = get_silly_info();
        const auto token = std::string{"PICKET FENCE\n"};
        const auto tempfile = msc::test::tempfile{".json"};
        if (HAVE_POSIX_FILENO) {
            const auto fhptr = std::unique_ptr<FILE, int(*)(FILE*)>{fopen(tempfile.filename().c_str(), "w"), &fclose};
            if (fhptr.get() == nullptr) MSC_ERROR("Cannot open temporary file");
            const auto fd = fileno(fhptr.get());
            if (fd < 3) MSC_ERROR("File descriptor is too small");
            MSC_REQUIRE_EQ(write(fd, token.data(), token.size()), token.size());  // Make sure we don't truncate
            msc::print_meta(info, msc::file::from_descriptor(fd));
            MSC_REQUIRE_EQ(write(fd, token.data(), token.size()), token.size());  // Make sure we don't close
        }
        MSC_REQUIRE_EQ(token + stringize(info) + token, tempfile.read());
    }

    MSC_AUTO_TEST_CASE(meta_descriptor_posix_badfd)
    {
        MSC_SKIP_UNLESS(HAVE_POSIX_WRITE);
        MSC_REQUIRE_EXCEPTION(std::system_error, msc::print_meta(get_silly_info(), msc::file::from_descriptor(12345)));
    }

}  // namespace /*anonymous*/
