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

#include "rlimits.hxx"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "testaux/envguard.hxx"
#include "testaux/tempfile.hxx"
#include "unittest.hxx"

extern "C" char **environ;

namespace /*anonymous*/
{

    const char environment_variables[][24] = {
        "MSC_LIMIT_CORE",
        "MSC_LIMIT_CPU",
        "MSC_LIMIT_DATA",
        "MSC_LIMIT_FSIZE",
        "MSC_LIMIT_NOFILE",
        "MSC_LIMIT_STACK",
        "MSC_LIMIT_AS",
    };

    // This function unsets all `MSC_LIMIT_*` variables that might currently be set in the environment.
    // No mechanism is provided to undo this change again.
    void clear_environment()
    {
        using namespace std;
        char **envptr = nullptr;
        if constexpr (HAVE_POSIX_ENVIRON) {
            envptr = environ;
        }
        if (envptr == nullptr) {
            const auto ec = std::error_code{ENOSYS, std::system_category()};
            throw std::system_error{ec, "Cannot scan environment variables"};
        }
        // Cache the variables we have to unset so we don't modify the environment while we're iterating over it.
        auto todo = std::vector<std::string>{};
        const auto prefix = "MSC_LIMIT_";
        const auto prelen = strlen(prefix);
        for (auto ptr = envptr; *ptr != nullptr; ++ptr) {
            if (strncmp(*ptr, prefix, prelen) == 0) {
                const auto endptr = strchr(*ptr, '=');
                todo.emplace_back(*ptr, endptr);
            }
        }
        if constexpr (HAVE_POSIX_UNSETENV) {
            for (const auto& var : todo) {
                //std::clog << "Unsetting environment variable " << var << " ...\n";
                if (unsetenv(var.c_str()) != 0) {
                    const auto ec = std::error_code{errno, std::system_category()};
                    throw std::system_error{ec, "Cannot unset environment variable: " + var};
                }
            }
        } else {
            const auto ec = std::error_code{ENOSYS, std::system_category()};
            throw std::system_error{ec, "Cannot unset environment variables"};
        }
    }

    template <typename FwdIterT, typename PredT>
    bool only_some_of(const FwdIterT first, const FwdIterT last, const PredT predicate)
    {
        return !std::none_of(first, last, predicate) && !std::all_of(first, last, predicate);
    }

    MSC_AUTO_TEST_CASE(parse_environment_success)
    {
        MSC_SKIP_UNLESS(HAVE_POSIX_ENVIRON && HAVE_POSIX_UNSETENV);
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        clear_environment();
        for (const auto envvar : environment_variables) {
            for (const auto envval : { "12345", "123456", "1234567", "NONE" }) {
                auto guard = msc::test::envguard{envvar};
                guard.set(envval);
                try {
                    msc::set_resource_limits();
                } catch (const std::system_error&) {
                    // Never mind that.
                }
            }
        }
    }

    MSC_AUTO_TEST_CASE(parse_environment_failure)
    {
        MSC_SKIP_UNLESS(HAVE_POSIX_ENVIRON && HAVE_POSIX_UNSETENV);
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        clear_environment();
        for (const auto envvar : environment_variables) {
            for (const auto envval : { "", "five", "NINER!", "-1", "Next Tuesday", "...", "\t\r\v\n\b\a" }) {
                auto guard = msc::test::envguard{envvar};
                guard.set(envval);
                MSC_REQUIRE_EXCEPTION(std::invalid_argument, msc::set_resource_limits());
            }
        }
    }

    MSC_AUTO_TEST_CASE(non_posix_graceful_degradation)
    {
        MSC_SKIP_IF(HAVE_POSIX_GETRLIMIT && HAVE_POSIX_SETRLIMIT);
        MSC_SKIP_UNLESS(HAVE_POSIX_ENVIRON && HAVE_POSIX_UNSETENV);
        clear_environment();
        MSC_REQUIRE_EXCEPTION(std::system_error, msc::set_resource_limits());
    }

    MSC_AUTO_TEST_CASE(posix_do_nothing)
    {
        MSC_SKIP_UNLESS(HAVE_POSIX_GETRLIMIT && HAVE_POSIX_SETRLIMIT);
        MSC_SKIP_UNLESS(HAVE_POSIX_ENVIRON && HAVE_POSIX_UNSETENV);
        clear_environment();
        msc::set_resource_limits();
    }

    MSC_AUTO_TEST_CASE(posix_do_something)
    {
        MSC_SKIP_UNLESS(HAVE_POSIX_GETRLIMIT && HAVE_POSIX_SETRLIMIT);
        MSC_SKIP_UNLESS(HAVE_POSIX_ENVIRON && HAVE_POSIX_UNSETENV);
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        clear_environment();
        const auto george = [](auto&& thing){ return static_cast<bool>(thing); };
        const auto cstd_fclose = [](std::FILE *fh){ std::fclose(fh); };
        const auto envvar = "MSC_LIMIT_NOFILE";
        constexpr auto limit = 10;
        const msc::test::tempfile tempfiles[limit];
        {
            auto guard = msc::test::envguard{envvar};
            guard.set(std::to_string(limit));
            msc::set_resource_limits();
            std::vector<std::unique_ptr<std::FILE, decltype(cstd_fclose)>> handles{};
            for (const auto& temp : tempfiles) {
                handles.emplace_back(std::fopen(temp.filename().c_str(), "w"), cstd_fclose);
            }
            MSC_REQUIRE(only_some_of(std::begin(handles), std::end(handles), george));
        }
        {
            auto guard = msc::test::envguard{envvar};
            guard.unset();  // leave previously set limit in effect
            msc::set_resource_limits();
            std::vector<std::unique_ptr<std::FILE, decltype(cstd_fclose)>> handles{};
            for (const auto& temp : tempfiles) {
                handles.emplace_back(std::fopen(temp.filename().c_str(), "w"), cstd_fclose);
            }
            MSC_REQUIRE(only_some_of(std::begin(handles), std::end(handles), george));
        }
        {
            auto guard = msc::test::envguard{envvar};
            guard.set("NONE");  // lift previously set limit again
            msc::set_resource_limits();
            std::vector<std::unique_ptr<std::FILE, decltype(cstd_fclose)>> handles{};
            for (const auto& temp : tempfiles) {
                handles.emplace_back(std::fopen(temp.filename().c_str(), "w"), cstd_fclose);
            }
            MSC_REQUIRE(std::all_of(std::begin(handles), std::end(handles), george));
        }
    }

}  // namespace /*anonymous*/
