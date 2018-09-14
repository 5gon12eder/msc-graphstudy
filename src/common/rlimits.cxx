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
#include <config.h>
#endif

#include "rlimits.hxx"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#if HAVE_POSIX_GETRLIMIT && HAVE_POSIX_SETRLIMIT
#  include <sys/resource.h>
#  define RESOURCE_CONSTANT_DICT_ENTRY(RES)  {#RES, RLIMIT_##RES}
#else
#  define RESOURCE_CONSTANT_DICT_ENTRY(RES)  {#RES, -1}
#endif

#ifdef RLIM_INFINITY
#  define RESOURCE_CONSTANT_UNLIMITED RLIM_INFINITY
#else
#  define RESOURCE_CONSTANT_UNLIMITED ULLONG_MAX
#endif

#include "strings.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        using limit_type = unsigned long long;

        std::optional<limit_type> parse_environment(const char *const envvar)
        {
            const auto envval = std::getenv(envvar);
            if (envval == nullptr) {
                return std::nullopt;
            }
            if (envval[0] == '\0') {
                throw std::invalid_argument{concat("Environment variable ", envvar, " must not be empty")};
            }
            if (std::strcmp(envval, "NONE") == 0) {
                return RESOURCE_CONSTANT_UNLIMITED;
            }
            if (std::isdigit(envval[0])) {
                auto endptr = static_cast<char*>(nullptr);
                const auto value = std::strtoull(envval, &endptr, 10);
                if ((endptr != nullptr) && (*endptr == '\0')) {
                    return value;
                }
            }
            throw std::invalid_argument{
                concat("Environment variable ", envvar, " cannot be parsed as non-negative decimal integer: ", envval)
            };
        }

        int lookup_resource(const std::string_view resname)
        {
            static const auto lookuptable = std::map<std::string_view, int>{
                RESOURCE_CONSTANT_DICT_ENTRY(CORE),
                RESOURCE_CONSTANT_DICT_ENTRY(CPU),
                RESOURCE_CONSTANT_DICT_ENTRY(DATA),
                RESOURCE_CONSTANT_DICT_ENTRY(FSIZE),
                RESOURCE_CONSTANT_DICT_ENTRY(NOFILE),
                RESOURCE_CONSTANT_DICT_ENTRY(STACK),
                RESOURCE_CONSTANT_DICT_ENTRY(AS),
            };
            return lookuptable.at(resname);
        }

        [[noreturn]] void fail_set_limit(const std::error_code ec,
                                         const std::string_view resname,
                                         const limit_type limit)
        {
            const auto message = concat("Cannot set resource limit for ", resname, " to ", std::to_string(limit));
            throw std::system_error{ec, message};
        }

        void set_limit(const std::string_view resname, const limit_type limit)
        {
#if HAVE_POSIX_GETRLIMIT && HAVE_POSIX_SETRLIMIT
            const auto res = lookup_resource(resname);
            auto spec = rlimit{};
            if (getrlimit(res, &spec) != 0) {
                const auto ec = std::error_code{errno, std::system_category()};
                fail_set_limit(ec, resname, limit);
            }
            spec.rlim_cur = std::min<limit_type>(spec.rlim_max, limit);
            if (setrlimit(res, &spec) != 0) {
                const auto ec = std::error_code{errno, std::system_category()};
                fail_set_limit(ec, resname, limit);
            }
#else
            const auto ec = std::error_code{ENOSYS, std::system_category()};
            fail_set_limit(ec, resname, limit);
#endif
        }

    }  // namespace /*anonymous*/

    void set_resource_limits()
    {
        const std::string_view resources[] = {"CORE", "CPU", "DATA", "FSIZE", "NOFILE", "STACK", "AS"};
        auto limits = std::map<std::string_view, limit_type>{};
        for (const auto& res : resources) {
            const auto envvar = concat("MSC_LIMIT_", res);
            if (const auto limit = parse_environment(envvar.c_str())) {
                limits[res] = limit.value();
            }
        }
        for (const auto& kv : limits) {
            set_limit(kv.first, kv.second);
        }
    }

}  // namespace msc
