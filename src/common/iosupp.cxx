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

#include "iosupp.hxx"

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/newline.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include "strings.hxx"
#include "useful.hxx"

namespace io = boost::iostreams;

namespace msc
{

    std::string_view check_filename(const std::string_view filename)
    {
        if (filename.empty()
            || std::isspace(static_cast<unsigned char>(filename.front()))
            || std::isspace(static_cast<unsigned char>(filename.back()))) {
            throw std::invalid_argument{"File names must not be empty and cannot have leading or trailing spaces"};
        }
        return filename;
    }

    int check_descriptor(const int descriptor)
    {
        if (descriptor < 0) {
            throw std::invalid_argument{"File descriptors cannot be negative"};
        }
        return descriptor;
    }

    std::pair<std::string_view, std::string_view> split_filename(const std::string_view filename) noexcept
    {
        const auto pos = filename.find_last_of(':');
        if (pos == std::string_view::npos) {
            return {filename, ""};
        }
        return {filename.substr(0, pos), filename.substr(pos + 1, filename.length())};
    }

    bool is_nullio(const std::string_view filename) noexcept
    {
        return ((filename == "") || (filename == "NULL"));
    }

    bool is_stdio(const std::string_view filename) noexcept
    {
        return ((filename == "-") || (filename == "STDIO"));
    }

    std::optional<int> is_fdno(const std::string_view filename) noexcept
    {
        return parse_decimal_number(filename);
    }

    std::string canonical_io_name(const file& thefile)
    {
        switch (thefile.terminal()) {
        case terminals::null:
            return "/dev/null";
        case terminals::stdio:
            switch (thefile.mode()) {
            case 'I': return "/dev/stdin";
            case 'O': return "/dev/stdout";
            default:  return "/dev/stdio";  // made-up
            }
        case terminals::descriptor:
            return concat("/proc/self/fd/", std::to_string(thefile.descriptor()));
        case terminals::file:
            return thefile.filename();
        }
        return "???";
    }

    [[noreturn]] void report_io_error(const std::string_view filename, const std::string_view message)
    {
        const auto ec = std::error_code{EIO, std::system_category()};
        throw std::system_error{ec, concat(filename, ": ", message)};
    }

    compressions guess_compression(const std::string_view filename) noexcept
    {
        if (endswith(filename, ".gz" )) { return compressions::gzip;  }
        if (endswith(filename, ".bz2")) { return compressions::bzip2; }
        return compressions::none;
    }

    std::string prepare_stream(io::filtering_istream& stream, const input_file& src)
    {
        auto name = canonical_io_name(src);
        //stream.push(io::newline_filter{io::newline::posix | io::newline::final_newline});
        switch (src.compression()) {
        case compressions::none:
            break;
        case compressions::gzip:
            stream.push(io::gzip_decompressor{});
            break;
        case compressions::bzip2:
            stream.push(io::bzip2_decompressor{});
            break;
        case compressions::automatic:
            MSC_NOT_REACHED();
            break;
        }
        switch (src.terminal()) {
        case terminals::null:
            stream.push(io::null_source{});
            break;
        case terminals::stdio:
            stream.push(io::file_descriptor_source{STDIN_FILENO, io::never_close_handle});
            break;
        case terminals::descriptor:
            stream.push(io::file_descriptor_source{src.descriptor(), io::never_close_handle});
            break;
        case terminals::file:
            stream.push(io::file_descriptor_source{src.filename()});
            break;
        }
        if (!stream) {
            report_io_error(name, "Cannot open file for reading");
        }
        return name;
    }

    std::string prepare_stream(io::filtering_ostream& stream, const output_file& dst)
    {
        auto name = canonical_io_name(dst);
        //stream.push(io::newline_filter{io::newline::posix | io::newline::final_newline});
        switch (dst.compression()) {
        case compressions::none:
            break;
        case compressions::gzip:
            stream.push(io::gzip_compressor{});
            break;
        case compressions::bzip2:
            stream.push(io::bzip2_compressor{});
            break;
        case compressions::automatic:
            MSC_NOT_REACHED();
            break;
        }
        switch (dst.terminal()) {
        case terminals::null:
            stream.push(io::null_sink{});
            break;
        case terminals::stdio:
            stream.push(io::file_descriptor_sink{STDOUT_FILENO, io::never_close_handle});
            break;
        case terminals::descriptor:
            stream.push(io::file_descriptor_sink{dst.descriptor(), io::never_close_handle});
            break;
        case terminals::file:
            stream.push(io::file_descriptor_sink{dst.filename()});
            break;
        }
        if (!stream) {
            report_io_error(name, "Cannot open file for writing");
        }
        return name;
    }

}  // namespace msc
