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

#include "cli.hxx"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <optional>
#include <ostream>
#include <system_error>
#include <utility>

#if __has_include(<unistd.h>)
#  include <unistd.h>
#endif

#if __has_include(<stropts.h>)
#  include <stropts.h>
#endif

#if __has_include(<sys/ioctl.h>)
#  include <sys/ioctl.h>
#endif

#include <ogdf/basic/Logger.h>

#include "rlimits.hxx"
#include "useful.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        auto next_printable(const std::string_view text, const std::string_view::size_type offset)
            -> std::string_view::size_type
        {
            for (auto i = offset; i < text.size(); ++i) {
                if (!std::isspace(static_cast<unsigned char>(text[i]))) {
                    return i;
                }
            }
            return std::string_view::npos;
        }

        auto next_break_point(const std::string_view text, const std::string_view::size_type offset)
            -> std::string_view::size_type
        {
            for (auto i = offset; i < text.size(); ++i) {
                if (std::isspace(static_cast<unsigned char>(text[i]))) {
                    return i;
                }
            }
            return (offset < text.size()) ? text.size() : std::string_view::npos;
        }

        auto wrap_line(std::ostream& os,
                       const std::string_view indent,
                       const std::string_view text,
                       const std::string_view::size_type offset,
                       const std::string_view::size_type width)
            -> std::string_view::size_type
        {
            assert(width > indent.size());
            const auto first = next_printable(text, offset);
            auto last = next_break_point(text, next_printable(text, first));
            while (last < text.size()) {
                const auto next = next_break_point(text, next_printable(text, last));
                if (next - first > width - indent.size()) { break; }
                last = next;
            }
            if (first != last) {
                os << indent << std::string_view(text.data() + first, last - first) << '\n';
            }
            return last;
        }

        class pretty_printer final
        {
        public:

            pretty_printer(const int width, const int first_column = 24) : _width{width}, _first_column{first_column}
            {
                assert(width > 0);
                assert(first_column > 0);
                assert(width > 2 * first_column);
                _indent = std::string(2, ' ');
                _subsequent_indent = std::string(first_column, ' ');
            }

            void operator()(std::ostream& os) const
            {
                os << '\n';
            }

            void operator()(std::ostream& os, const std::string_view text) const
            {
                assert(text.find_first_of("\a\b\t\n\v\f\r") == std::string_view::npos);
                _print_wrapped(os, text);
            }

            void operator()(std::ostream& os, const std::string_view title, const std::string_view description) const
            {
                assert(title.find_first_of("\a\b\t\n\v\f\r") == std::string_view::npos);
                assert(description.find_first_of("\a\b\t\n\v\f\r") == std::string_view::npos);
                const auto leftover = _first_column - static_cast<std::ptrdiff_t>(_indent.size() + title.size());
                if (leftover > 0) {
                    //os << _indent << std::setw(_first_column) << std::left << title;
                    const auto introitus = concat(_indent, title, std::string(leftover, ' '));
                    _print_wrapped(os, description, introitus, _subsequent_indent);
                } else {
                    os << _indent << title << '\n';
                    _print_wrapped(os, description, _subsequent_indent, _subsequent_indent);
                }
            }

        private:

            std::string _indent{};
            std::string _subsequent_indent{};
            int _width{};
            int _first_column{};

            void _print_wrapped(std::ostream& os,
                                const std::string_view text,
                                const std::string_view indent_first,
                                const std::string_view indent_after) const
            {
                for (auto offset = wrap_line(os, indent_first, text, 0, _width);
                     offset < text.length();
                     offset = wrap_line(os, indent_after, text, offset, _width)) { /* keep on wrapping */ }
            }

            void _print_wrapped(std::ostream& os, const std::string_view text, const std::string_view indent) const
            {
                _print_wrapped(os, text, indent, indent);
            }

            void _print_wrapped(std::ostream& os, const std::string_view text) const
            {
                _print_wrapped(os, text, "", "");
            }

        };  // class pretty_printer final

        std::optional<int> get_terminal_width_kernel(std::error_code& ec)
        {
            using namespace std;
            ec.clear();
            if constexpr (HAVE_POSIX_IOCTL && HAVE_LINUX_WINSIZE && HAVE_POSIX_STDOUT_FILENO && HAVE_LINUX_TIOCGWINSZ) {
                auto info = winsize{};
                if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &info) != -1) {
                    return info.ws_col;
                }
                ec.assign(errno, std::system_category());
            } else {
                ec.assign(ENOSYS, std::system_category());
            }
            return std::nullopt;
        }

        std::optional<int> get_terminal_width_environment(std::error_code& ec)
        {
            using namespace std;
            ec.clear();
            if constexpr (HAVE_POSIX_GETENV) {
                if (const auto token = getenv("COLUMNS")) {
                    const auto value = parse_decimal_number(token);
                    if (value.value_or(-1) > 0) { return value; }
                    ec.assign(EINVAL, std::system_category());
                } else {
                    ec.assign(ENOKEY, std::system_category());
                }
            } else {
                ec.assign(ENOSYS, std::system_category());
            }
            return std::nullopt;
        }

    }  // namespace /*anonymous*/

    int guess_terminal_width(const int fallback) noexcept
    {
        auto ec = std::error_code{};
        if (const auto optval = get_terminal_width_kernel(ec)) {
            return *optval;
        }
        if (const auto optval = get_terminal_width_environment(ec)) {
            return *optval;
        }
        return fallback;
    }

    namespace detail::cli
    {

        void init_cli_base(cli_base& cli)
        {
            cli.epilog.push_back("Please visit " MSC_PACKAGE_URL " for more information.");
            cli.environ["MSC_RANDOM_SEED"] = "deterministic random seed";
            cli.environ["MSC_LIMIT_${RES}"] = "set resource limit for resource ${RES}";
        }

        namespace /*anonymous*/
        {

            std::optional<char> get_flag(const po::option_description& opt)
            {
                // This is an awfully dirty hack that apparently is necessary because Boost.ProgramOptions sucks.
                const auto ouch = opt.format_name();
                const auto offset = ouch.find_first_not_of(' ');
                if ((offset != std::string::npos) && (ouch.length() >= offset + 2)) {
                    if ((ouch[offset] == '-') && (ouch[offset + 1] != '-')) {
                        return ouch[1];
                    }
                }
                return std::nullopt;
            }

            std::string format_short_option_synopsis(const po::option_description& opt)
            {
                auto pretty = std::string{};
                if (const auto flag = get_flag(opt)) {
                    pretty.push_back('-');
                    pretty.push_back(*flag);
                }
                return pretty;
            }

            std::string format_long_option_synopsis(const po::option_description& opt, const bool bracketize = true)
            {
                const auto sem = opt.semantic();
                auto pretty = std::string{};
                if (bracketize && !sem->is_required()) { pretty.push_back('['); }
                pretty.append("--").append(opt.long_name());
                if ((sem->min_tokens() == 0u) && (sem->max_tokens() == 0u)) {
                    // Option takes no parameters
                } else if ((sem->min_tokens() == 0u) && (sem->max_tokens() == 1u)) {
                    pretty.append("[=").append(sem->name()).append("]");
                } else if ((sem->min_tokens() == 0u) && (sem->max_tokens() > 1u)) {
                    pretty.append("[=").append(sem->name()).append("...]");
                } else if ((sem->min_tokens() == 1u) && (sem->max_tokens() == 1u)) {
                    pretty.append("=").append(sem->name());
                } else if ((sem->min_tokens() == 1u) && (sem->max_tokens() > 1u)) {
                    pretty.append("=").append(sem->name()).append("...");
                } else {
                    throw std::invalid_argument{"Confused by required number of tokens for option"};
                }
                if (bracketize && !sem->is_required()) { pretty.push_back(']'); }
                return pretty;
            }

            std::string format_combined_option_synopsis(const po::option_description& opt)
            {
                const auto shortsyn = format_short_option_synopsis(opt);
                const auto longsyn = format_long_option_synopsis(opt, false);
                if (shortsyn.empty()) {
                    return std::string(4, ' ') + longsyn;
                } else {
                    assert(shortsyn.length() == 2);
                    return concat(shortsyn, ", ", longsyn);
                }
            }

            std::string format_argument_synopsis(const po::option_description& arg, const bool bracketize = true)
            {
                const auto sem = arg.semantic();
                auto pretty = std::string{};
                if (bracketize && !sem->is_required()) { pretty.push_back('['); }
                pretty.append(arg.format_parameter());
                if (bracketize && !sem->is_required()) { pretty.push_back(']'); }
                return pretty;
            }

        }  // namespace /*anonymous*/

        void add_version_and_help(po::options_description& description)
        {
            description.add_options()
                ("version", "show version information and exit")
                ("help", "show usage information and exit");
        }

        void handle_version_and_help(po::variables_map& varmap,
                                     const po::options_description& arguments,
                                     const po::options_description& options,
                                     const cli_base& cli)
        {
            const auto printer = pretty_printer{std::max(50, guess_terminal_width())};
            if (varmap.count("version")) {
                printer(std::cout, cli.prog);
                printer(std::cout, "Copyright (C) " MSC_PACKAGE_YEAR " " MSC_PACKAGE_AUTHOR);
                printer(std::cout,
                        "This is free software; see the source for copying conditions.  There is NO"
                        " warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. ");
                printer(std::cout,
                        "Please report bugs to " MSC_PACKAGE_BUGREPORT " or visit "
                        MSC_PACKAGE_URL " for more information.");
                throw system_exit{};
            }
            if (varmap.count("help")) {
                if (!cli.usage.empty()) {
                    std::cout << "usage: " << cli.usage << "\n";
                } else {
                    std::cout << "usage: " << cli.prog;
                    for (const auto& opt : options.options()) {
                        if (opt->semantic()->is_required()) {
                            std::cout << " " << format_long_option_synopsis(*opt);
                        }
                    }
                    for (const auto& arg : arguments.options()) {
                        std::cout << " " << format_argument_synopsis(*arg);
                    }
                    printer(std::cout);
                }
                for (const auto& paragraph : cli.help) {
                    printer(std::cout);
                    printer(std::cout, paragraph);
                }
                if (!arguments.options().empty()) {
                    printer(std::cout);
                    printer(std::cout, "Arguments:");
                    for (const auto& arg : arguments.options()) {
                        const auto synopsis = format_argument_synopsis(*arg, false);
                        printer(std::cout, synopsis, arg->description());
                    }
                }
                if (!options.options().empty()) {
                    printer(std::cout);
                    printer(std::cout, "Options:");
                    for (const auto& opt : options.options()) {
                        const auto synopsis = format_combined_option_synopsis(*opt);
                        printer(std::cout, synopsis, opt->description());
                    }
                }
                if (!cli.environ.empty()) {
                    printer(std::cout);
                    printer(std::cout, "Environment Variables:");
                    for (const auto& [envvar, description] : cli.environ) {
                        printer(std::cout, envvar, description);
                    }
                }
                for (const auto& paragraph : cli.epilog) {
                    printer(std::cout);
                    printer(std::cout, paragraph);
                }
                std::cout << std::flush;
                throw system_exit{};
            }
        }

        void before_main()
        {
            for (const auto lc : { "POSIX", "C" }) {
                if (std::setlocale(LC_ALL, lc)) {
                    break;
                }
            }
            set_resource_limits();
            ogdf::Logger::setWorldStream(std::clog);
        }

        void after_main()
        {
            check_stdio(std::cin, std::cout);
        }

    }  // namespace detail::cli

    namespace /*anonymous*/
    {

        std::optional<std::system_error> check_stdio_impl(std::istream& input, std::ostream& output) noexcept
        {
            if (!input) {
                const auto ec = std::error_code{EIO, std::system_category()};
                return std::system_error{ec, "Cannot read from standard input"};
            }
            if (!output) {
                const auto ec = std::error_code{EIO, std::system_category()};
                return std::system_error{ec, "Cannot write to standard output"};
            }
            return std::nullopt;
        }

    }  // namespace /*anonymous*/

    void check_stdio(std::istream& input, std::ostream& output)
    {
        if (const auto opt = check_stdio_impl(input, output)) {
            throw *opt;
        }
    }

    bool check_stdio(std::istream& input, std::ostream& output, std::string& message) noexcept
    {
        if (const auto opt = check_stdio_impl(input, output)) {
            message.assign(opt->what());
            return false;
        }
        return true;
    }

    const std::string& helptext_file_name_expansion()
    {
        static const auto text = std::string{
            "This program might produce multiple output files.  If the '--output=FILE' option is given, any '%' in"
            " FILE will be substituted by a token derived from the current iteration."
        };
        return text;
    }

    namespace /*anonymous*/
    {

        output_file expand_filename_rate(const output_file& dst, const double rate)
        {
            assert((rate >= 0.0) && (rate <= 1.0));
            const auto digits = 5;
            const auto multiply = std::pow(10.0, digits - 1);
            const auto thisstep = static_cast<int>(std::round(multiply * rate));
            auto formatted = std::to_string(thisstep);
            formatted.insert(0, digits - formatted.length(), '0');
            auto thefile = output_file{};
            if (dst.terminal() == terminals::file) {
                auto expanded = std::string{};
                for (const auto c : dst.filename()) {
                    if (c == '%') {
                        expanded.append(formatted);
                    } else {
                        expanded.push_back(c);
                    }
                }
                thefile.assign<terminals::file>(expanded, dst.compression());
            } else {
                thefile.assign(dst);
            }
            return thefile;
        }

    } // namespace /*anonymous*/

    output_file cli_parameters_interpolation::expand_filename(const double degree) const
    {
        return expand_filename_rate(this->output, degree);
    }

    output_file cli_parameters_worsening::expand_filename(const double degree) const
    {
        return expand_filename_rate(this->output, degree);
    }

    std::size_t cli_parameters_property::iterations() const noexcept
    {
        return std::max({std::size_t{1}, this->width.size(), this->bins.size()});
    }

    output_file expand_filename(const output_file& pattern, const std::size_t iteration)
    {
        auto thefile = output_file{};
        if (pattern.terminal() == terminals::file) {
            auto expanded = std::string{};
            for (const auto c : pattern.filename()) {
                if (c == '%') {
                    expanded.append(std::to_string(iteration));
                } else {
                    expanded.push_back(c);
                }
            }
            thefile.assign<terminals::file>(expanded, pattern.compression());
        } else {
            thefile.assign(pattern);
        }
        return thefile;
    }

    output_file expand_filename(const output_file& pattern, const std::size_t major, const std::size_t minor)
    {
        auto tally = 0;
        auto thefile = output_file{};
        if (pattern.terminal() == terminals::file) {
            auto expanded = std::string{};
            for (const auto c : pattern.filename()) {
                if (c == '%') {
                    switch (tally++) {
                    case 0:
                        expanded.append(std::to_string(major));
                        break;
                    case 1:
                        expanded.append(std::to_string(minor));
                        break;
                    default:
                        throw std::invalid_argument{"Too many '%' characters in file name template"};
                    }
                } else {
                    expanded.push_back(c);
                }
            }
            thefile.assign<terminals::file>(expanded, pattern.compression());
        } else {
            thefile.assign(pattern);
        }
        return thefile;
    }

}  // namespace msc
