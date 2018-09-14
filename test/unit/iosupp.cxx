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

#include "iosupp.hxx"

#include "unittest.hxx"

namespace /*anonymous*/
{

    MSC_AUTO_TEST_CASE(check_filename_okay)
    {
        const std::string_view names[] = {
            "-", "file.txt", "/path/to/file.txt", "./file.txt", "sub/file", "sub/", ".",
            "./-", "/+", "with spaces", ".1", ".hidden", "/", "42", "+1", "-13",
        };
        for (const auto& name : names) {
            msc::check_filename(name);
        }
    }

    MSC_AUTO_TEST_CASE(check_filename_notok)
    {
        const std::string_view names[] = { "", " ", "         ", "\t", "\n", " foo", "foo ", " foo " };
        for (const auto& name : names) {
            MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::check_filename(name));
        }
    }

    MSC_AUTO_TEST_CASE(check_descriptor_okay)
    {
        const int values[] = { 0, 1, 2, 3, 4, 5, 10, 42, 111 };
        for (const auto value : values) {
            msc::check_descriptor(value);
        }
    }

    MSC_AUTO_TEST_CASE(check_descriptor_notok)
    {
        const int values[] = { -1, -2, -3, -10000 };
        for (const auto value : values) {
            MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::check_descriptor(value));
        }
    }

    MSC_AUTO_TEST_CASE(split_filename_first)
    {
        using namespace std::string_view_literals;
        MSC_REQUIRE_EQ(""sv, msc::split_filename("").first);
        MSC_REQUIRE_EQ(""sv, msc::split_filename(":").first);
        MSC_REQUIRE_EQ(""sv, msc::split_filename(":abc").first);
        MSC_REQUIRE_EQ("file"sv, msc::split_filename("file").first);
        MSC_REQUIRE_EQ("path/to/file.txt.gz"sv, msc::split_filename("path/to/file.txt.gz").first);
        MSC_REQUIRE_EQ("/abs/path/to/file.txt.gz"sv, msc::split_filename("/abs/path/to/file.txt.gz").first);
        MSC_REQUIRE_EQ("path/to/file.txt.gz"sv, msc::split_filename("path/to/file.txt.gz:bzip2").first);
        MSC_REQUIRE_EQ("A:\\file.dat"sv, msc::split_filename("A:\\file.dat:").first);
        MSC_REQUIRE_EQ("A:ff::enmilchmann"sv, msc::split_filename("A:ff::enmilchmann:KNILCH").first);
        MSC_REQUIRE_EQ(":::::::::"sv, msc::split_filename("::::::::::").first);
        MSC_REQUIRE_EQ("42"sv, msc::split_filename("42").first);
        MSC_REQUIRE_EQ("42"sv, msc::split_filename("42:").first);
        MSC_REQUIRE_EQ("42"sv, msc::split_filename("42:bytes").first);
        MSC_REQUIRE_EQ("42"sv, msc::split_filename("42:0").first);
    }

    MSC_AUTO_TEST_CASE(split_filename_second)
    {
        using namespace std::string_view_literals;
        MSC_REQUIRE_EQ(""sv, msc::split_filename("").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename(":").second);
        MSC_REQUIRE_EQ("abc"sv, msc::split_filename(":abc").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("file").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("path/to/file.txt.gz").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("/abs/path/to/file.txt.gz").second);
        MSC_REQUIRE_EQ("bzip2"sv, msc::split_filename("path/to/file.txt.gz:bzip2").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("A:\\file.dat:").second);
        MSC_REQUIRE_EQ("KNILCH"sv, msc::split_filename("A:ff::enmilchmann:KNILCH").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("::::::::::").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("42").second);
        MSC_REQUIRE_EQ(""sv, msc::split_filename("42:").second);
        MSC_REQUIRE_EQ("bytes"sv, msc::split_filename("42:bytes").second);
        MSC_REQUIRE_EQ("0"sv, msc::split_filename("42:0").second);
    }

    MSC_AUTO_TEST_CASE(is_nullio)
    {
        MSC_REQUIRE_EQ(true, msc::is_nullio(""));
        MSC_REQUIRE_EQ(false, msc::is_nullio(":abc"));
        MSC_REQUIRE_EQ(true, msc::is_nullio("NULL"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("NULL:abc"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("-"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("-:abc"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("STDIO"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("STDIO:abc"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("STDIN"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("STDOUT"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("STDERR"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("stdio"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("stdin"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("stdout"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("stderr"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("/dev/stdin"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("/dev/stdout"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("/dev/stderr"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("file.txt"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("file.txt:abc"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("./file.txt"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("dir/"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("/"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("."));
        MSC_REQUIRE_EQ(false, msc::is_nullio("0"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("1"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("+1"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("-7"));
        MSC_REQUIRE_EQ(false, msc::is_nullio("42"));
    }

    MSC_AUTO_TEST_CASE(is_stdio)
    {
        MSC_REQUIRE_EQ(false, msc::is_stdio(""));
        MSC_REQUIRE_EQ(false, msc::is_stdio("NULL"));
        MSC_REQUIRE_EQ(true, msc::is_stdio("-"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("-:"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("-:stuff"));
        MSC_REQUIRE_EQ(true, msc::is_stdio("STDIO"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("STDIO:"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("STDIN"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("STDOUT"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("STDERR"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("stdio"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("stdin"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("stdout"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("stderr"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("/dev/stdin"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("/dev/stdout"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("/dev/stderr"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("file.txt"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("./file.txt"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("dir/"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("/"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("."));
        MSC_REQUIRE_EQ(false, msc::is_stdio("0"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("1"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("+1"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("-7"));
        MSC_REQUIRE_EQ(false, msc::is_stdio("42"));
    }

    MSC_AUTO_TEST_CASE(is_fdno)
    {
        MSC_REQUIRE(!msc::is_fdno(""));
        MSC_REQUIRE(!msc::is_fdno("-"));
        MSC_REQUIRE(!msc::is_fdno("STDIO"));
        MSC_REQUIRE(!msc::is_fdno("NULL"));
        MSC_REQUIRE(!msc::is_fdno("NULL:gz"));
        MSC_REQUIRE_EQ(0, msc::is_fdno("0").value());
        MSC_REQUIRE_EQ(1, msc::is_fdno("1").value());
        MSC_REQUIRE_EQ(2, msc::is_fdno("2").value());
        MSC_REQUIRE_EQ(3, msc::is_fdno("3").value());
        MSC_REQUIRE_EQ(42, msc::is_fdno("42").value());
        MSC_REQUIRE(!msc::is_fdno("42:"));
        MSC_REQUIRE(!msc::is_fdno("42:7z"));
        MSC_REQUIRE(!msc::is_fdno("-1"));
        MSC_REQUIRE(!msc::is_fdno("+23"));
        MSC_REQUIRE(!msc::is_fdno("."));
        MSC_REQUIRE(!msc::is_fdno(".1"));
        MSC_REQUIRE(!msc::is_fdno("1.1"));
        MSC_REQUIRE(!msc::is_fdno("/1"));
        MSC_REQUIRE(!msc::is_fdno("./1"));
        MSC_REQUIRE(!msc::is_fdno("file.txt"));
        MSC_REQUIRE(!msc::is_fdno("./file.txt"));
        MSC_REQUIRE(!msc::is_fdno("dir/"));
        MSC_REQUIRE(!msc::is_fdno("/"));
        MSC_REQUIRE(!msc::is_fdno("/path/to/file.txt"));
    }

    MSC_AUTO_TEST_CASE(guess_compression)
    {
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression(""));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("-"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("NULL"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("STDIO"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("file"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("file.txt"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("what/a/beautiful/picture.jpg"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("/absolutely/terriffic.pdf"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("/"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("bad/"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("bad.gz/"));
        MSC_REQUIRE_EQ(msc::compressions::none, msc::guess_compression("nope.gz.b64"));
        MSC_REQUIRE_EQ(msc::compressions::gzip, msc::guess_compression("file.gz"));
        MSC_REQUIRE_EQ(msc::compressions::gzip, msc::guess_compression("file.tar.gz"));
        MSC_REQUIRE_EQ(msc::compressions::gzip, msc::guess_compression("file.jpg.b64.gz"));
        MSC_REQUIRE_EQ(msc::compressions::gzip, msc::guess_compression("/file.gz"));
        MSC_REQUIRE_EQ(msc::compressions::gzip, msc::guess_compression("good/file.gz"));
    }

}  // namespace /*anonymous*/
