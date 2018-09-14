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
 * @file stdio.hxx
 *
 * @brief
 *     Redirecting and capturing standard I/O streams for testing.
 *
 */

#ifndef MSC_TESTAUX_STDIO_HXX
#define MSC_TESTAUX_STDIO_HXX

#include <iostream>
#include <sstream>

namespace msc::test
{

    /**
     * @brief
     *     Temporarily redirects `std::cout`, `std::cerr` and `std::cin` to / from memory buffers.
     *
     * @warning
     *     This does not affect data written via the I/O mechanisms provided by the C `stdio.h` facility (that is,
     *     writing to `stdout` using the `fwrite` function and its friends) or by the POXIX `unistd.h` facility (tat is,
     *     writing to `STDOUT_FILENO` using the `write` function and its friends).  Data written to `std::clog` should
     *     also not be affected which is good because it can be used to report test status while I/O is captured.
     *
     */
    class capture_stdio final
    {
    public:

        /**
         * @brief
         *     Redirects standard I/O streams to / from memory buffers.
         *
         * @param input
         *     content to provide as standard input
         *
         */
        explicit capture_stdio(const std::string& input = "");

        /**
         * @brief
         *     Restores the old destination of standard output.
         *
         */
        ~capture_stdio() noexcept;

        /**
         * @brief
         *     Deleted copy constructor.
         *
         * @param other
         *     *N/A*
         *
         */
        capture_stdio(const capture_stdio& other) = delete;

        /**
         * @brief
         *     Deleted move constructor.
         *
         * @param other
         *     *N/A*
         *
         */
        capture_stdio(capture_stdio&& other) noexcept = delete;

        /**
         * @brief
         *     Deleted copy assignment operator.
         *
         * @param other
         *     *N/A*
         *
         * @returns
         *     *N/A*
         *
         */
        capture_stdio& operator=(const capture_stdio& other) = delete;

        /**
         * @brief
         *     Deleted move assignment operator.
         *
         * @param other
         *     *N/A*
         *
         * @returns
         *     *N/A*
         *
         */
        capture_stdio& operator=(capture_stdio&& other) noexcept = delete;

        /**
         * @brief
         *     Returns the contents written to standard output so far (since construction of this guard).
         *
         * @returns
         *     captured standard output
         *
         */
        std::string get_stdout() const;

        /**
         * @brief
         *     Returns the contents written to standard error output so far (since construction of this guard).
         *
         * @returns
         *     captured standard error output
         *
         */
        std::string get_stderr() const;

    private:

        /** @brief Old `std::cin` buffer to restore.  */
        decltype(std::cout.rdbuf()) _oldbufin{};

        /** @brief Old `std::cout` buffer to restore.  */
        decltype(std::cout.rdbuf()) _oldbufout{};

        /** @brief Old `std::cerr` buffer to restore.  */
        decltype(std::cerr.rdbuf()) _oldbuferr{};

        /** @brief Internal memory buffer for `std::cin`.  */
        std::ostringstream _tempin{};

        /** @brief Internal memory buffer for `std::cout`.  */
        std::ostringstream _tempout{};

        /** @brief Internal memory buffer for `std::cerr`.  */
        std::ostringstream _temperr{};

    };  // capture_stdio

}  // namespace msc::test

#endif  // !defined(MSC_TESTAUX_STDIO_HXX)
