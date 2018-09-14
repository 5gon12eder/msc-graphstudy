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

#ifndef MSC_INCLUDED_FROM_UNITTEST_HXX
#  error "Never `#include <unittest.txx>` directly; `#include <unittest.hxx>` instead."
#endif

#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dc7c12e5b72012bbc7e47c3e5436903c
{

    //// GLOBAL TEST REGISTRATION ////

    std::map<std::string, std::function<void()>>& get_all_test_cases() noexcept;

    template <typename KeyT, typename ValT>
    std::size_t register_test_case(KeyT&& key, ValT&& val)
    {
        get_all_test_cases()[std::forward<KeyT>(key)] = std::forward<ValT>(val);
        return get_all_test_cases().size();
    }

    [[nodiscard]] int run_registred_unit_tests(const int argc, const char *const *const argv);

    template <typename... FuncTs>
    [[nodiscard]] int run_unit_tests(const int argc, const char *const *const argv, FuncTs&&... funcs)
    {
        get_all_test_cases().clear();
        const auto functions = std::vector<std::function<void()>> { std::forward<FuncTs>(funcs)... };
        for (auto i = std::size_t{}; i < functions.size(); ++i) {
            register_test_case(std::to_string(i + 1), functions[i]);
        }
        return run_registred_unit_tests(argc, argv);
    }

    //// PRETTY PRINTING ////

    std::string repr(float val);
    std::string repr(double val);
    std::string repr(long double val);
    std::string repr(bool val);
    std::string repr(std::string_view val);

    template <typename T, typename = void>
    struct has_repr : std::false_type { };

    template <typename T>
    struct has_repr<T, std::enable_if_t<std::is_same_v<std::string, decltype(repr(std::declval<T>()))>>>
        : std::true_type { };

    template <typename T, typename = void>
    struct has_ostr : std::false_type { };

    template <typename T>
    struct has_ostr<T, decltype((void) (std::declval<std::ostream&>() << std::declval<T>()))>
        : std::true_type { };

    template <typename T>
    struct pretty
    {
        const T* wrapped{};

        pretty(const T& value) noexcept : wrapped{&value}
        {
        }

        friend std::ostream& operator<<(std::ostream& os, const pretty<T>& st)
        {
            if constexpr (has_repr<T>::value) {
                return os << repr(*st.wrapped);
            } else if constexpr (has_ostr<T>::value) {
                return os << *st.wrapped;
            } else if constexpr (std::is_enum<T>::value) {
                using raw_type = std::underlying_type_t<T>;
                const auto raw = static_cast<raw_type>(*st.wrapped);
                return os << raw;
            } else {
                return os << "{?}";
            }
        }
    };

    inline std::string_view prettyrelop(const std::equal_to<>)      noexcept { return " == "; }
    inline std::string_view prettyrelop(const std::not_equal_to<>)  noexcept { return " != "; }
    inline std::string_view prettyrelop(const std::greater<>)       noexcept { return " > ";  }
    inline std::string_view prettyrelop(const std::less<>)          noexcept { return " < ";  }
    inline std::string_view prettyrelop(const std::greater_equal<>) noexcept { return " >= "; }
    inline std::string_view prettyrelop(const std::less_equal<>)    noexcept { return " <= "; }

    //// SKIPPING AND FAILING TESTS ////

    struct abnormal_return
    {
        std::string filename{};
        int lineno{};
        std::string message{};
        std::vector<std::string> moreinfo{};

        abnormal_return() noexcept
        {
        }

        abnormal_return(const std::string_view file, const int line, const std::string_view msg) : lineno{line}
        {
            this->filename.assign(file.data(), file.size());
            this->message.assign(msg.data(), msg.size());
        }

        void assign(const std::string_view file, const int line, const std::string_view msg)
        {
            this->filename.assign(file.data(), file.size());
            this->lineno = line;
            this->message.assign(msg.data(), msg.size());
            this->moreinfo.clear();
        }

        template <typename... Ts>
        void amend(Ts&&... args)
        {
            auto oss = std::ostringstream{};
            (oss << ... << std::forward<Ts>(args));
            this->moreinfo.push_back(oss.str());
        }

    };  // struct abnormal_return

    struct skipped_t final : abnormal_return { using abnormal_return::abnormal_return; };
    struct failed_t  final : abnormal_return { using abnormal_return::abnormal_return; };
    struct error_t   final : abnormal_return { using abnormal_return::abnormal_return; };

    std::ostream& operator<<(std::ostream& ostr, const skipped_t& info);
    std::ostream& operator<<(std::ostream& ostr, const failed_t&  info);
    std::ostream& operator<<(std::ostream& ostr, const error_t&   info);

    [[noreturn]] void skip_msg(const std::string_view filename, const int lineno, const std::string_view message);
    [[noreturn]] void fail_msg(const std::string_view filename, const int lineno, const std::string_view message);
    [[noreturn]] void err_msg (const std::string_view filename, const int lineno, const std::string_view message);

    //// ACTUAL ASSERTIONS ////

    template <typename T>
    void check_require(T&& condition,
                       const std::string_view filename,
                       const int lineno,
                       const std::string_view test)
    {
        if (!static_cast<bool>(condition)) {
            auto info = failed_t{filename, lineno, "Condition not true"};
            info.amend("condition: ", test);
            info.amend("   --->    ", pretty(condition));
            throw info;
        }
    }

    template <typename RelopT, typename T1, typename T2>
    void check_require_relop(const RelopT op,
                             T1&& lhs,
                             T2&& rhs,
                             const std::string_view filename,
                             const int lineno,
                             const std::string_view lhsexpr,
                             const std::string_view rhsexpr)
    {
        if (!op(lhs, rhs)) {
            auto info = failed_t{filename, lineno, "Comparison not true"};
            info.amend(lhsexpr, prettyrelop(op), rhsexpr);
            info.amend("lhs: ", pretty(lhs));
            info.amend("rhs: ", pretty(rhs));
            throw info;
        }
    }

    template <typename T1, typename T2>
    void check_require_implies(T1&& lhs, T2&& rhs,
                               const std::string_view filename,
                               const int lineno,
                               const std::string_view lhsexpr,
                               const std::string_view rhsexpr)
    {
        if (bool(lhs) && !bool(rhs)) {
            auto info = failed_t{filename, lineno, "Implication is not true"};
            info.amend(lhsexpr, " => ", rhsexpr);
            info.amend("lhs: ", pretty(lhs));
            info.amend("rhs: ", pretty(rhs));
            throw info;
        }
    }

    template <typename T>
    std::enable_if_t<std::is_floating_point_v<T>>
    check_require_close(const T tol, const T lhs, const T rhs,
                        const std::string_view filename,
                        const int lineno,
                        const std::string_view lhsexpr,
                        const std::string_view rhsexpr)
    {
        if (!std::isfinite(tol) || (tol < 0)) {
            auto info = error_t{filename, lineno, "A non-negative tolerance is required: " + repr(tol)};
            throw info;
        }
        if (!std::isfinite(lhs) || !std::isfinite(rhs) || (std::abs(lhs - rhs) > tol)) {
            auto info = failed_t{filename, lineno, "Operands not within tolerance of " + repr(tol)};
            info.amend("expected:   ", lhsexpr);
            info.amend("   --->     ", pretty(lhs));
            info.amend("actual:     ", rhsexpr);
            info.amend("   --->     ", pretty(rhs));
            info.amend("difference: ", pretty(std::abs(lhs - rhs)));
            throw info;
        }
    }

    void check_require_match(const std::string_view pattern,
                             const std::string_view text,
                             const std::string_view filename,
                             const int lineno,
                             const std::string_view patternexpr,
                             const std::string_view textexpr);

}  // namespace dc7c12e5b72012bbc7e47c3e5436903c
