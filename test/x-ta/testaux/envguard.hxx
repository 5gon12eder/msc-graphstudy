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
 * @file envguard.hxx
 *
 * @brief
 *     Scope guards for restoring environment variables.
 *
 */

#ifndef MSC_TESTAUX_ENVGUARD_HXX
#define MSC_TESTAUX_ENVGUARD_HXX

#include <optional>
#include <string>

namespace msc::test
{

    /**
     * @brief
     *     Scope guard that restores an environment variable.
     *
     * This feature is only available if the POSIX functions `setenv` and `getenv` are provided by `<stdlib.h>`.
     * Otherwise, the constructor of this class will always throw an exception.
     *
     */
    class envguard final
    {
    public:

        /**
         * @brief
         *     Constructs a new guard object that will restore the environment variable `name`.
         *
         * If `name` is not a valid name for an environment variable, in particular if it contains an embedded NUL byte,
         * the behavior is undefined.
         *
         * @param name
         *     name of the environment variable to restore in the destructor
         *
         * @throws std::exception
         *     if the class invariant cannot be established
         *
         */
        envguard(std::string name);

        /**
         * @brief
         *     Restores the value of the environment variable as it was on construction.
         *
         * If the varieble was not set in the first place, it will be unset again.  If the restore operation fails, an
         * error message will be printed to standard error output and no further action taken.
         *
         */
        ~envguard();

        envguard(const envguard&) = delete;

        envguard& operator=(const envguard&) = delete;

        /**
         * @brief
         *     Returns the name of the guarded environment variable.
         *
         * @returns
         *     name of the environment variable
         *
         */
        const std::string& name() const noexcept
        {
            return _name;
        }

        /**
         * @brief
         *     Sets the guarded environment variable.
         *
         * If `value` contains an embedded NUL byte, the behavior is undefined.
         *
         * @param value
         *     value to set the environment variable to
         *
         * @throws std::system_error
         *     if there is an error updating the environment
         *
         */
        void set(const std::string& value);

        /**
         * @brief
         *     Unsets (clears) the guarded environment variable.
         *
         * @throws std::system_error
         *     if there is an error updating the environment
         *
         */
        void unset();

        /**
         * @brief
         *     Tells whether this class can perform its job and will not always simply fail.
         *
         * @returns
         *     whether this class is usable
         *
         */
        static bool can_be_used() noexcept;

    private:

        /** @brief Name of the guarded environment varieble. */
        std::string _name{};

        /** @brief Previous value of the guarded environment variable (if any). */
        std::optional<std::string> _previous{};

    };  // class envguard

}  // namespace msc::test

#endif  // !defined(MSC_TESTAUX_ENVGUARD_HXX)
