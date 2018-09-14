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

#include "testaux/tempfile.hxx"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <iterator>
#include <memory>
#include <random>
#include <system_error>

#define MSC_FILESYSTEM_IMPL_NONE   0
#define MSC_FILESYSTEM_IMPL_STD    1
#define MSC_FILESYSTEM_IMPL_BOOST  2

#ifndef MSC_FILESYSTEM_IMPL
#  if __has_include(<filesystem>)
#    define MSC_FILESYSTEM_IMPL MSC_FILESYSTEM_IMPL_STD
#  elif __has_include(<boost/filesystem.hpp>)
#    define MSC_FILESYSTEM_IMPL MSC_FILESYSTEM_IMPL_BOOST
#  else
#    define MSC_FILESYSTEM_IMPL MSC_FILESYSTEM_IMPL_NONE
#  endif
#endif

#if MSC_FILESYSTEM_IMPL == MSC_FILESYSTEM_IMPL_STD
#  include <filesystem>
   namespace fs = std::filesystem;
   using fs_error_code = std::error_code;
#elif MSC_FILESYSTEM_IMPL == MSC_FILESYSTEM_IMPL_BOOST
#  include <boost/filesystem.hpp>
   namespace fs = boost::filesystem;
   using fs_error_code = boost::system::error_code;
#else
#  error "No file-system library (C++17 or Boost) found / selected"
#endif

namespace msc::test
{

    struct fclose_deleter
    {
        void operator()(std::FILE* fp) noexcept
        {
            if (std::fclose(fp) < 0) {
                const auto ec = std::error_code{errno, std::system_category()};
                std::fprintf(stderr, "ERROR: Cannot close file: %s\n", ec.message().c_str());
            }
        }
    };

    using file_handle = std::unique_ptr<std::FILE, fclose_deleter>;

    namespace /*anonymous*/
    {

        std::string random_string()
        {
            const auto characters = std::string_view{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
            auto result = std::string{};
            auto rndeng = std::random_device{};
            auto rnddst = std::uniform_int_distribution<std::size_t>{0, characters.size() - 1};
            auto lambda = [&characters, &rndeng, &rnddst](){ return characters[rnddst(rndeng)]; };
            std::generate_n(std::back_inserter(result), 16, lambda);
            return result;
        }

        file_handle open_file(const std::string& filename, const std::string& mode)
        {
            auto fhptr = file_handle{std::fopen(filename.c_str(), mode.c_str())};
            if (!fhptr) {
                const auto ec = std::error_code{errno, std::system_category()};
                throw std::system_error{ec, "Cannot open file: " + filename};
            }
            return fhptr;
        }

        void close_file(file_handle& fhptr, const std::string& name = "{unknown file name}")
        {
            if (fhptr) {
                if (std::fclose(fhptr.release()) != 0) {
                    const auto ec = std::error_code{errno, std::system_category()};
                    throw std::system_error{ec, "Cannot close file: " + name};
                }
            }
        }

    }  // namespace /*anonymous*/

    tempfile::tempfile(const std::string_view suffix)
    {
        // I don't want to depend on string.hxx in the unit test support library so let's do the verbose and inefficient
        // thing here...
        const auto name = "temp-" + random_string() + std::string{suffix.data(), suffix.length()};
        const auto path = fs::temp_directory_path() / name;
        _filename = path.string();
        open_file(_filename, "wb");
    }

    tempfile::~tempfile() noexcept
    {
        const auto path = fs::path{_filename};
        auto ec = fs_error_code{};
        if (!fs::remove(path, ec)) {
            std::fprintf(stderr, "error: %s: Cannot remove temporary file: %s\n",
                         _filename.c_str(), ec.message().c_str());
        }
    }

    std::string tempfile::read() const
    {
        return readfile(_filename);
    }

    std::string readfile(const std::string_view filename)
    {
        const auto name = std::string{filename.data(), filename.size()};
        auto result = std::string{};
        auto buffer = std::array<char, 512>{};
        auto fhptr = open_file(name, "rb");
        do {
            const auto count = std::fread(buffer.data(), 1, buffer.size(), fhptr.get());
            if (std::ferror(fhptr.get())) {
                const auto ec = std::error_code{errno, std::system_category()};
                throw std::system_error{ec, "Cannot read file: " + name};
            }
            result.append(buffer.data(), count);
        } while (!std::feof(fhptr.get()));
        close_file(fhptr, name);
        return result;
    }

}  // namespace msc::test
