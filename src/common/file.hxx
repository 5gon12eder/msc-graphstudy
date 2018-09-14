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
 * @file file.hxx
 *
 * @brief
 *     Abstractions for specifying input sources and output destinations in a type-safe manner.
 *
 */

#ifndef MSC_FILE_HXX
#define MSC_FILE_HXX

#include <string>
#include <string_view>

#include "enums/compressions.hxx"
#include "enums/terminals.hxx"

namespace msc
{

    /**
     * @brief
     *     An I/O terminal with no associated direction.
     */
    class file
    {
    public:

        /**
         * @brief
         *     Constructs a null terimal with no compression.
         *
         */
        file() noexcept;

        /**
         * @brief
         *     Constructs a teriminal from a textual specification.
         *
         * @param spec
         *     augmented file name (see component-level documentation for allowed syntax)
         *
         * @throws std::invalid_argument
         *     if an invalid compression algorithm was specified
         *
         */
        explicit file(std::string_view spec);

        /**
         * @brief
         *     Constructs a termimal from a file name and an explicit compression specification.
         *
         * If `compression == compressions::automatic`, the suffix of `filename` will be used to guess the compression
         * to use.
         *
         * @param filename
         *     verbatim file name
         *
         * @param compression
         *     compression to use
         *
         * @throws std::invalid_argument
         *     if it is figured out that the provided `filename` cannot possibly be a sane file name
         *
         * @returns
         *     the constructed file
         *
         */
        static file from_filename(std::string_view filename, compressions compression = compressions::automatic);

        /**
         * @brief
         *     Constructs a termimal from a file descriptor and an explicit compression specification.
         *
         * `compressions::automatic` will be treated as `compressions::none`.
         *
         * @param descriptor
         *     descriptor of existing open file (that shall not be closed when done using)
         *
         * @param compression
         *     compression to use
         *
         * @returns
         *     the constructed file
         *
         * @throws std::invalid_argument
         *     if `descriptor < 0` (use `from_null` to construct an invalid `file`)
         *
         */
        static file from_descriptor(int descriptor, compressions compression = compressions::automatic);

        /**
         * @brief
         *     Constructs a termimal referring to a null terminal using the specified compression.
         *
         * `compressions::automatic` will be treated as `compressions::none`.
         *
         * @param compression
         *     compression to use
         *
         * @returns
         *     the constructed file
         *
         */
        static file from_null(compressions compression = compressions::automatic);

        /**
         * @brief
         *     Constructs a termimal referring to a standard I/O terminal using the specified compression.
         *
         * `compressions::automatic` will be treated as `compressions::none`.
         *
         * @param compression
         *     compression to use
         *
         * @returns
         *     the constructed file
         *
         */
        static file from_stdio(compressions compression = compressions::automatic);

        /**
         * @brief
         *     Changes the value of the file.
         *
         * This function has the same effect as if a `file` were first constructed using the `from_*` factory function
         * forwarding it the provided arguments and then assigned to `*this`.
         *
         * @tparam Term
         *     selects the new terminal type after assignment
         *
         * @tparam ArgTs...
         *     types of any additional arguments
         *
         * @param args
         *     any additional arguments
         *
         */
        template <terminals Term = terminals{}, typename... ArgTs>
        void assign(ArgTs&&... args);

        /**
         * @brief
         *     Changes the value of the `file`.
         *
         * This function has the same effect as if a `file` were fitst constructed using the single-argument constructor
         * and then assigned to `*this`.
         *
         * @param spec
         *     augmented file name (see component-level documentation for allowed syntax)
         *
         * @throws std::invalid_argument
         *     if an invalid compression algorithm was specified
         *
         */
        void assign_from_spec(std::string_view spec);

        /** @brief Virtual default destructor.  */
        virtual ~file() noexcept = default;

        /**
         * @brief
         *     Returns the type of I/O terminal.
         *
         * @returns
         *     I/O terminal type
         *
         */
        terminals terminal() const noexcept;

        /**
         * @brief
         *     Returns the compression algorithm.
         *
         * This function will never return `compressions::automatic`.
         *
         * @returns
         *     compression algorithm
         *
         */
        compressions compression() const noexcept;

        /**
         * @brief
         *     Returns the file name referred to (if any).
         *
         * @returns
         *     file name or empty string if none
         *
         */
        const std::string& filename() const noexcept;

        /**
         * @brief
         *     Returns the file descriptor referred to.
         *
         * @returns
         *     file descriptor or -1 if none
         *
         */
        int descriptor() const noexcept;

        /**
         * @brief
         *     Returns `'I'` if the file is for reading, `'O'` if it is for writing and `0` otherwise.
         *
         * @returns
         *     the constant `0`
         *
         */
        virtual int mode() const noexcept;

    private:

        /** @brief Terminal type.  */
        terminals _terminal{terminals::null};

        /** @brief Compression algorithm (never `compressions::automatic`).  */
        compressions _compression{compressions::none};

        /** @brief File name referred to (if any).  */
        std::string _filename{};

        /** @brief File descriptor referred to (if any).  */
        int _descriptor{-1};

    };  // class file

    /**
     * @brief
     *     Compares two `file`s for equality.
     *
     * Two files are considered equal if and only if they both refer to the same terminal type, file name, file
     * descriptor and compression.  The "mode" is not considered.
     *
     * @param lhs
     *     first file to compare
     *
     * @param rhs
     *     second file to compare
     *
     * @returns
     *     whether the two files refer to the same I/O file
     *
     */
    bool operator==(const file& lhs, const file& rhs) noexcept;

    /**
     * @brief
     *     Compares two `file`s for inequality.
     *
     * @param lhs
     *     first file to compare
     *
     * @param rhs
     *     second file to compare
     *
     * @returns
     *     `!(lhs == rhs)`
     *
     */
    bool operator!=(const file& lhs, const file& rhs) noexcept;

    /**
     * @brief
     *     An input terminal.
     *
     */
    struct input_file final : public file
    {

        using file::file;

        /**
         * @brief
         *     Converts a `file` with no assigned I/O direction to a dedicated `input_file`.
         *
         * @param src
         *     source
         *
         */
        input_file(file src);

        /**
         * @brief
         *     Returns `'I'` indicating that the file is for reading.
         *
         * @returns
         *     the constant `'I'`
         *
         */
        virtual int mode() const noexcept override;

    };  // struct input_file

    /**
     * @brief
     *     An output terminal.
     *
     */
    struct output_file final : public file
    {

        using file::file;

        /**
         * @brief
         *     Converts a `file` with no assigned I/O direction to a dedicated `output_file`.
         *
         * @param dst
         *     destination
         *
         */
        output_file(file dst);

        /**
         * @brief
         *     Returns `'O'` indicating that the file is for writing.
         *
         * @returns
         *     the constant `'O'`
         *
         */
        virtual int mode() const noexcept override;

    };  // struct output_file

}  // namespace msc

#define MSC_INCLUDED_FROM_FILE_HXX
#include "file.txx"
#undef MSC_INCLUDED_FROM_FILE_HXX

#endif  // !defined(MSC_FILE_HXX)
