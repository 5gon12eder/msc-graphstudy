// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2016 Moritz Klammler <moritz.klammler@student.kit.edu>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "unittest.hxx"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <regex>
#include <sstream>
#include <utility>

// We use this crappy name space (which happens to be the hex representation of the MD5 hash of the string "unit test"
// to make it crystal clear that these symbols shall not be considered part of any API.

namespace dc7c12e5b72012bbc7e47c3e5436903c
{

    namespace /*anonymous*/
    {

        class msg_guard
        {
        public:

            msg_guard(std::ostream& ostr, const bool bold = false, const int color = 1) noexcept
                : _ostr{ostr}, _bold{bold}
            {
                assert((color > 0) && (color < 9));
                if (use_ansi_escapes()) {
                    if (_bold) {
                        _ostr << "\033[1m";
                    }
                    _ostr << "\033[3" << color << "m";
                }
            }

            ~msg_guard() noexcept
            {
                if (use_ansi_escapes()) {
                    _ostr << (_bold ? "\033[22m\033[39m\n" : "\033[39m\n");
                } else {
                    _ostr << '\n';
                }
            }

            msg_guard(const msg_guard&) = delete;
            msg_guard(msg_guard&&) noexcept = delete;
            msg_guard& operator=(const msg_guard&) = delete;
            msg_guard& operator=(msg_guard&&) = delete;

            static bool use_ansi_escapes() noexcept
            {
                static const bool choice = [](){
                    if (const auto envval = std::getenv("MSC_TEST_ANSI_TERMINAL")) {
                        return (envval[0] != '\0');
                    }
                    return false;
                }();
                return choice;
            }

        private:

            std::ostream& _ostr;
            bool _bold{};

        };  // class msg_guard

        template <typename T>
        std::string floating_point_repr(const T x, const char *const suffix = "")
        {
            constexpr auto digits = std::numeric_limits<T>::max_digits10;
            auto oss = std::ostringstream{};
            oss << std::setprecision(digits) << x << suffix;
            oss << " (" << std::hexfloat << x << ")";
            return oss.str();
        }

    }  // namespace /*anonymous*/

    std::string repr(const bool x)
    {
        return x ? "TRUE" : "FALSE";
    }

    std::string repr(const float x)
    {
        return floating_point_repr(x, "f");
    }

    std::string repr(const double x)
    {
        return floating_point_repr(x);
    }

    std::string repr(const long double x)
    {
        return floating_point_repr(x, "lf");
    }

    std::string repr(const std::string_view str)
    {
        auto oss = std::ostringstream{};
        oss << '"';
        for (auto it = std::begin(str); it != std::end(str); ++it) {
            const auto c = 0 + static_cast<unsigned char>(*it);
            switch (c) {
            case ' ': oss << " "; break;
            case '\"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\a': oss << "\\a"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            case '\v': oss << "\\v"; break;
            default:
                if (std::isalnum(c) || std::ispunct(c)) {
                    oss << *it;
                } else {
                    oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << c;
                }
            }
        }
        oss << "\" (" << str.size() << " bytes)";
        return oss.str();
    }

    namespace /*anonymous*/
    {

        std::ostream& print_abnormal_return(std::ostream& ostr, const abnormal_return& info, const int color)
        {
            {
                const auto guard = msg_guard{ostr, true, color};
                ostr << info.filename << ":" << info.lineno << ": " << info.message;
            }
            for (const auto& line : info.moreinfo) {
                const auto guard = msg_guard{ostr, false, color};
                ostr << line;
            }
            return ostr;
        }

        int get_output_color(const char *const envvar, const int fallback) noexcept
        {
            if (const auto envval = std::getenv(envvar)) {
                if ((envval[0] >= '0') && (envval[0] <= '9') && (envval[1] == '\0')) {
                    return envval[0] - '0';
                }
            }
            return fallback;
        }

    }  // namespace /*anonymous*/

    std::ostream& operator<<(std::ostream& ostr, const skipped_t& info)
    {
        static const auto color = get_output_color("MSC_TEST_ANSI_COLOR_SKIPPED", 3 /* yellow */);
        return print_abnormal_return(ostr, info, color);
    }

    std::ostream& operator<<(std::ostream& ostr, const failed_t& info)
    {
        static const auto color = get_output_color("MSC_TEST_ANSI_COLOR_FAILED", 1 /* red */);
        return print_abnormal_return(ostr, info, color);
    }

    std::ostream& operator<<(std::ostream& ostr, const error_t& info)
    {
        static const auto color = get_output_color("MSC_TEST_ANSI_COLOR_ERROR", 5 /* purple */);
        return print_abnormal_return(ostr, info, color);
    }

    [[noreturn]] void skip_msg(const std::string_view filename, const int lineno, const std::string_view message)
    {
        throw skipped_t{filename, lineno, message};
    }

    [[noreturn]] void fail_msg(const std::string_view filename, const int lineno, const std::string_view message)
    {
        throw failed_t{filename, lineno, message};
    }

    [[noreturn]] void err_msg(const std::string_view filename, const int lineno, const std::string_view message)
    {
        throw error_t{filename, lineno, message};
    }

    void check_require_match(const std::string_view pattern,
                             const std::string_view text,
                             const std::string_view filename,
                             const int lineno,
                             const std::string_view patternexpr,
                             const std::string_view textexpr)
    {
        const auto regex = std::regex{std::begin(pattern), std::end(pattern)};
        auto match = std::cmatch{};
        if (!std::regex_match(std::begin(text), std::end(text), match, regex)) {
            auto info = failed_t{filename, lineno, "Regular expression not matched"};
            info.amend("pattern: ", patternexpr);
            info.amend("   --->  ", repr(pattern));
            info.amend("text:    ", textexpr);
            info.amend("   --->  ", repr(text));
            throw info;
        }
    }

    std::map<std::string, std::function<void()>>& get_all_test_cases() noexcept
    {
        static auto testsuite = std::map<std::string, std::function<void()>>{};
        return testsuite;
    }

    [[nodiscard]] int run_registred_unit_tests(const int argc, const char *const *const argv)
    {
        if (argc > 1) {
            std::clog << argv[0] << ": error: Too many arguments" << std::endl;
            return EXIT_FAILURE;
        }
        if (get_all_test_cases().empty()) {
            std::clog << argv[0] << ": error: There are no tests to run" << std::endl;
            return EXIT_FAILURE;
        }
        auto passed = 0;
        auto skipped = 0;
        auto failures = 0;
        auto errors = 0;
        auto total = 0;
        for (const auto& [name, func] : get_all_test_cases()) {
            std::clog << "running unit test " << name << " ... " << std::flush;
            try {
                func();
                ++passed;
                std::clog << "passed" << std::endl;
            } catch (const skipped_t& info) {
                ++skipped;
                std::clog << "skipped\n" << info << std::endl;
            } catch (const failed_t& info) {
                ++failures;
                std::clog << "failed\n" << info << std::endl;
            } catch (const error_t& info) {
                ++errors;
                std::clog << "error\n" << info << std::endl;
            } catch (const std::exception& e) {
                ++errors;
                std::clog << "error\n";
                {
                    const auto guard = msg_guard{std::clog, true};
                    std::clog << "Unexpected exception: " << e.what();
                }
                std::clog << std::endl;
            }
            ++total;
        }
        const auto precents = [total](const int n){
            return 100.0 * static_cast<double>(n) / static_cast<double>(total);
        };
        std::clog << std::setprecision(2) << std::fixed
                  << "Passed:   " << std::setw(10) << passed   << std::setw(10) << precents(passed)   << " %\n"
                  << "Skipped:  " << std::setw(10) << skipped  << std::setw(10) << precents(skipped)  << " %\n"
                  << "Failures: " << std::setw(10) << failures << std::setw(10) << precents(failures) << " %\n"
                  << "Errors:   " << std::setw(10) << errors   << std::setw(10) << precents(errors)   << " %\n"
                  << "Total:    " << std::setw(10) << total    << std::setw(10) << precents(total)    << " %\n"
                  << std::flush;
        return ((failures == 0) && (errors == 0)) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

}  // namespace dc7c12e5b72012bbc7e47c3e5436903c
