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
 * @file iosupp.hxx
 *
 * @brief
 *     Low-level utilities for implementing I/O functions.
 *
 */

#ifndef MSC_IOSUPP_HXX
#define MSC_IOSUPP_HXX

#include <optional>
#include <string_view>

#include <boost/iostreams/filtering_stream.hpp>

#include "file.hxx"

namespace msc
{

    /**
     * @brief
     *     Checks whether the given string can possibly be a sane file name.
     *
     * This function rejects only the empty string and strings with leading or trailing white-space.
     *
     * @param filename
     *     file name to check
     *
     * @returns
     *     its argument

     * @throws std::invalid_argument
     *     if the name was invalid
     *
     */
    std::string_view check_filename(std::string_view filename);

    /**
     * @brief
     *     Checks whether the given integer can possibly be a sane file descriptor.
     *
     * This function rejects only negative integers.
     *
     * @param descriptor
     *     file descriptor to check
     *
     * @returns
     *     its argument
     *
     * @throws std::invalid_argument
     *     if the integer was invalid
     *
     */
    int check_descriptor(int descriptor);

    /**
     * @brief
     *     Splits a file name into the actual file name and an optional compression specification.
     *
     * The file name is split at the last instance of a `:` character.  For example, `"STDIO:gzip"` will be split into
     * `{"STDIO", "gzip"}` and `"C:\\Documents\\file.txt:none"` into `{"C:\\Documents\\file.txt", "none"}`.
     *
     * No validation whatsoever on the compression specification is performed.
     *
     * @param filename
     *     file name to split
     *
     * @returns
     *     pair of actual file name and compression specification
     *
     */
    std::pair<std::string_view, std::string_view> split_filename(const std::string_view filename) noexcept;

    /**
     * @brief
     *     Tests whether a file name shall be interpreted as no input or output output.
     *
     * This function returns `true` for and only for the string `"NULL"` and the empty string.  It does *not* return
     * `true` for `"/dev/null"` or any integer.  These file names should not be treated specially because they are
     * not semantically equivalent.
     *
     * @param filename
     *     file name to check
     *
     * @returns
     *     whether the file name refers to no input or output
     *
     */
    bool is_nullio(std::string_view filename) noexcept;

    /**
     * @brief
     *     Tests whether a file name shall be interpreted as standard input or output.
     *
     * This function returns `true` for and only for the strings `"STDIO"` and `"-"`.  It does *not* return `true` for
     * `"/dev/std*"` or integers that happen to correspond to the respective file descriptors.  These file names should
     * not be treated specially because they are not semantically equivalent.
     *
     * @param filename
     *     file name to check
     *
     * @returns
     *     whether the file name refers to standard input or output
     *
     */
    bool is_stdio(std::string_view filename) noexcept;

    /**
     * @brief
     *     Tests whether a file name shall be interpreted as an open file descriptor and if so, its value.
     *
     * @param filename
     *     file name to check
     *
     * @returns
     *     the value of the file descriptor, if any
     *
     */
    std::optional<int> is_fdno(std::string_view filename) noexcept;

    /**
     * @brief
     *     Attempts to guess the compression applied to a file from its name.
     *
     * The following suffixes will be understood.
     *
     * <table>
     *   <tr>
     *     <th>Suffix</th>
     *     <th>Compression</th>
     *   </tr>
     *   <tr>
     *     <td>`*.gz`</td>
     *     <td>`compressions::gzip`</td>
     *   </tr>
     *   <tr>
     *     <td>`*.bz2`</td>
     *     <td>`compressions::bzip2`</td>
     *   </tr>
     * </table>
     *
     * If the `filename` does not match any of those extensions, `compressions::none` is returned.
     * Under no circumstances will this function return `compressions::automatic`.
     *
     * @param filename
     *     name of the file to guess about
     *
     * @returns
     *     inferred compression
     *
     */
    compressions guess_compression(const std::string_view filename) noexcept;

    /**
     * @brief
     *     Throws an exception to report an I/O error.
     *
     * @param filename
     *     informal name of the file on which the I/O operation failed
     *
     * @param message
     *     description of the error that occurred
     *
     * @throws std::system_error
     *     always
     *
     */
    [[noreturn]] void report_io_error(std::string_view filename, std::string_view message = "I/O error");

    /**
     * @brief
     *     Stacks together a chain of I/O devices and filters suitable for reading from the given source.
     *
     * @param stream
     *     default-constructed input stream to set up
     *
     * @param src
     *     source to read from
     *
     * @returns
     *     informal name of the input file
     *
     */
    std::string prepare_stream(boost::iostreams::filtering_istream& stream, const input_file& src);

    /**
     * @brief
     *     Stacks together a chain of I/O devices and filters suitable for writing to the given destination.
     *
     * @param stream
     *     default-constructed output stream to set up
     *
     * @param dst
     *     destination to write to
     *
     * @returns
     *     informal name of the output file
     *
     */
    std::string prepare_stream(boost::iostreams::filtering_ostream& stream, const output_file& dst);

}  // namespace msc

#endif  // !defined(MSC_IOSUPP_HXX)
