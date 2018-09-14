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
 * @file rlimits.hxx
 *
 * @brief
 *     Voluntary resource limits depending on various environment variables.
 *
 * The functionality of this component is only available under POSIX.  On other systems, no limits will be adjusted.
 *
 */

#ifndef MSC_RLIMITS_HXX
#define MSC_RLIMITS_HXX

namespace msc
{

    /**
     * @brief
     *     Sets (soft) resource limits according to environment variables.
     *
     * The environment variables `MSC_LIMIT_CORE`, `MSC_LIMIT_CPU`, `MSC_LIMIT_DATA`, `MSC_LIMIT_FSIZE`,
     * `MSC_LIMIT_NOFILE`, `MSC_LIMIT_STACK` and `MSC_LIMIT_AS` will be treated as integers that specify the respective
     * soft limit for the respective resource.  See `getrlimit(3p)` for a discussion of the semantics of these limits.
     *
     * If a variable is not set, the respective limit will not be touched and remain at the system's default or a
     * previously set limit.  If any of the variables is set to a value other than `NONE` but cannot be parsed as a
     * non-negative decimal integer or overflows the applicable range, an exception will be thrown and this function
     * will have no effect.  If a variable is set to the special string `NONE`, the soft limit for that resource will be
     * cleared (set to `RLIM_INFINITY`).  Depending on the previous limit in effect, this might not be the same as not
     * touching the limit (if the environment variable is not set at all).
     *
     * Hard limits will not be touched in any case.
     *
     * On non-POSIX systems, this function still parses the environment variables but always throws an exception and has
     * no effect.  Note that this means that the user's desire to limit a resource will not be silently ignored on some
     * systems, which is considered a feature.
     *
     * @throws std::invalid_argument
     *     if any of the influential environment variables is set to an invalid value
     *
     * @throws std::system_error
     *     if setting a resource limit fails (always thrown on on-POSIX systems unless a previous error occurs)
     *
     */
    void set_resource_limits();

}  // namespace msc

#endif  // !defined(MSC_RLIMITS_HXX)
