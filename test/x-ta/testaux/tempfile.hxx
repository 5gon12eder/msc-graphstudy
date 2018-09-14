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
 * @file tempfile.hxx
 *
 * @brief
 *     Temporary files.
 *
 */

#ifndef MSC_TESTAUX_TEMPFILE_HXX
#define MSC_TESTAUX_TEMPFILE_HXX

#include <string>
#include <string_view>
#include <utility>

namespace msc::test
{

    /**
     * @brief
     *     A pretty unsafe temporary file.
     *
     */
    class tempfile final
    {
    private:

        /**
         * @brief Constructs an empty `tempfile`. */
        tempfile(std::nullptr_t) noexcept
        {
        }

    public:

        /**
         * @brief
         *     Creates a new temporary file that hopefully doesn't overwrites any existing file.
         *
         * @param suffix
         *     suffix for the random file name
         *
         * @throws std::exception
         *     if there is an error creating the file
         *
         */
        tempfile(const std::string_view suffix = "");

        /**
         * @brief
         *     Removes the temporary file.
         *
         * If there is an error removing the file, an error message will be printed to standard error output and no
         * further action taken.
         *
         */
        ~tempfile() noexcept;

        /**
         * @brief
         *     Deleted copy constructor.
         *
         * @param other
         *     N/A
         *
         */
        tempfile(const tempfile& other) = delete;

        /**
         * @brief
         *     Transfers ownership of the temporary file.
         *
         * @param other
         *     `tempfile` to transfer ownership from
         *
         */
        tempfile(tempfile&& other) : _filename{nullptr}
        {
            using std::swap;
            swap(*this, other);
        }

        /**
         * @brief
         *     Deleted copy assignment operator.
         *
         * @param other
         *     N/A
         *
         * @returns
         *     N/A
         *
         */
        tempfile& operator=(const tempfile& other) = delete;

        /**
         * @brief
         *     Transfers ownership of the temporary file.
         *
         * @param other
         *     `tempfile` to transfer ownership from
         *
         * @returns
         *     reference to the new owner
         *
         */
        tempfile& operator=(tempfile&& other) noexcept
        {
            tempfile ephem{nullptr};
            swap(*this, ephem);
            swap(*this, other);
            return *this;
        }

        /**
         * @brief
         *     Returns the absolute file name of the temporary file.
         *
         * @returns
         *     absolute file name
         *
         */
        const std::string& filename() const noexcept
        {
            return _filename;
        }

        /**
         * @brief
         *     Returns the current contents of the temporary file.
         *
         * @returns
         *     file contents as string
         *
         * @throws std::system_error
         *     if there is an error reading the file
         *
         */
        std::string read() const;

        /**
         * @brief
         *     Exchanges the owned temporary file between two `tempfile`s.
         *
         * @param lhs
         *     first temporary file
         *
         * @param rhs
         *     second temporary file
         *
         */
        friend void swap(tempfile& lhs, tempfile& rhs) noexcept
        {
            using std::swap;
            swap(lhs._filename, rhs._filename);
        }

    private:

        /** @brief Absolute file name. */
        std::string _filename{};

    };  // class tempfile

    /**
     * @brief
     *     Reads a whole file into memory.
     *
     * The file is read in minary mode byte-for-byte.
     *
     * @returns
     *     data if the file
     *
     * @throws std::system_error
     *     if there is an error reading the file
     *
     */
    std::string readfile(const std::string_view filename);

}  // namespace msc::test

#endif  // !defined(MSC_TESTAUX_TEMPFILE_HXX)
