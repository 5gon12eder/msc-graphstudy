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

#include "file.hxx"

#include <stdexcept>

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(file_ctor_default)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_empty)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{""};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_filename)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"file"};
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ("file"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_filename_compress_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"graphstudy.tar.bz2"};
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::bzip2, file.compression());
        MSC_REQUIRE_EQ("graphstudy.tar.bz2"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_filename_compress_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"A:\\file.gz:BZIP2"};
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::bzip2, file.compression());
        MSC_REQUIRE_EQ("A:\\file.gz"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_filename_compress_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"A:\\file.gz:"};
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ("A:\\file.gz"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_filename_compress_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"A:\\file.gz:automatic"};
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ("A:\\file.gz"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_descriptor)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"1"};
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_descriptor_compress)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"3:gzip"};
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(3, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_null_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{""};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_null_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{":"};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_null_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"NULL"};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_null_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{":gzip"};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_null_5th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"NULL:bzip2"};
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::bzip2, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_stdio_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"-"};
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_stdio_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"-:gzip"};
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_stdio_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"STDIO"};
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_stdio_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"STDIO:"};
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_ctor_spec_stdio_5th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file{"STDIO:none"};
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_filename_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_filename("file.txt");
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ("file.txt"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_filename_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_filename("42", msc::compressions::gzip);
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ("42"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_filename_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_filename("A:\\ff:en\\milchmann:KNILCH");
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ("A:\\ff:en\\milchmann:KNILCH"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_filename_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_filename("file.txt.bz2");
        MSC_REQUIRE_EQ(msc::terminals::file, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::bzip2, file.compression());
        MSC_REQUIRE_EQ("file.txt.bz2"sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_filename_5th)
    {
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_filename("\tawkward"));
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_filename("awkward\n"));
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_filename("         "));
        for (const auto comp : msc::all_compressions()) {
            MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_filename("", comp));
        }
    }

    MSC_AUTO_TEST_CASE(file_from_descriptor_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_descriptor(42);
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(42, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_descriptor_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_descriptor(0, msc::compressions::none);
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(0, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_descriptor_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_descriptor(1, msc::compressions::automatic);
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_descriptor_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_descriptor(2, msc::compressions::bzip2);
        MSC_REQUIRE_EQ(msc::terminals::descriptor, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::bzip2, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(2, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_descriptor_5th)
    {
        MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_descriptor(-1));
        for (const auto comp : msc::all_compressions()) {
            MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::file::from_descriptor(-2, comp));
        }
    }

    MSC_AUTO_TEST_CASE(file_from_null_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_null();
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_null_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_null(msc::compressions::none);
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_null_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_null(msc::compressions::automatic);
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_null_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_null(msc::compressions::gzip);
        MSC_REQUIRE_EQ(msc::terminals::null, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_stdio_1st)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_stdio();
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_stdio_2nd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_stdio(msc::compressions::none);
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_stdio_3rd)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_stdio(msc::compressions::automatic);
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::none, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

    MSC_AUTO_TEST_CASE(file_from_stdio_4th)
    {
        using namespace std::string_view_literals;
        const auto file = msc::file::from_stdio(msc::compressions::gzip);
        MSC_REQUIRE_EQ(msc::terminals::stdio, file.terminal());
        MSC_REQUIRE_EQ(msc::compressions::gzip, file.compression());
        MSC_REQUIRE_EQ(""sv, file.filename());
        MSC_REQUIRE_EQ(-1, file.descriptor());
    }

}  // namespace /*anonymous*/
