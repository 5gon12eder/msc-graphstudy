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

#include "cli.hxx"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string_view>

#if __has_include(<sys/stat.h>)
#  include <sys/stat.h>
#endif

#if __has_include(<fcntl.h>)
#  include <fcntl.h>
#endif

#if __has_include(<unistd.h>)
#  include <unistd.h>
#endif

#include <ogdf/basic/graphics.h>

#include "file.hxx"
#include "testaux/envguard.hxx"
#include "testaux/stdio.hxx"
#include "unittest.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace /*anonymous*/
{

    template <std::size_t N>
    int cmdlen([[maybe_unused]] const char *const (&array)[N])
    {
        assert(std::all_of(array, array + N - 1, [](const char *const p){ return p != nullptr; }));
        assert(array[N - 1] == nullptr);
        assert(std::strcmp(array[0], __FILE__) == 0);
        return static_cast<int>(N - 1);
    }

    bool stdout_isatty()
    {
        using namespace std;
        auto ec = std::error_code{};
        if constexpr (HAVE_POSIX_ISATTY && HAVE_POSIX_STDOUT_FILENO) {
            if (isatty(STDOUT_FILENO)) {
                return true;
            } else if (errno == ENOTTY) {
                return false;
            } else {
                ec.assign(errno, std::system_category());
            }
        } else {
            ec.assign(ENOSYS, std::system_category());
        }
        throw std::system_error{ec, "Cannot decide whether standard output is a terminal"};
    }

    struct stdout_isatty_guard final
    {

        static constexpr bool can_be_used() noexcept
        {
            return HAVE_POSIX_STDOUT_FILENO
                && HAVE_POSIX_OPEN
                && HAVE_POSIX_OPEN_FLAGS
                && HAVE_POSIX_CLOSE
                && HAVE_POSIX_DUP
                && HAVE_POSIX_DUP2;
        }

        stdout_isatty_guard(const bool tty) noexcept(false)
        {
            using namespace std;
            const auto otherfile = tty ? "/dev/tty" : "/dev/null";
            auto ec = std::error_code{};
            auto msg = "Cannot temporarily switch standard output";
            flush_stdout(ec);
            if constexpr (can_be_used()) {
                if (!ec && ((_fd_old = dup(STDOUT_FILENO)) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot dup() original standard output file descriptor";
                }
                if (!ec && ((_fd_new = open(otherfile, O_RDWR | O_TRUNC | O_CLOEXEC)) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot open() new file for reading and writing";
                }
                if (!ec && (dup2(_fd_new, STDOUT_FILENO) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot dup2() file descriptor to become new standard output";
                }
            } else {
                ec.assign(ENOSYS, std::system_category());
            }
            if (ec) {
                throw std::system_error{ec, msg};
            }
        }

        ~stdout_isatty_guard() noexcept(false)
        {
            auto ec = std::error_code{};
            auto msg = "Cannot switch standard output back";
            flush_stdout(ec);
            if constexpr (can_be_used()) {
                if (!ec && (dup2(_fd_old, STDOUT_FILENO) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot dup2() original file descriptor for standard output to restore";
                }
                if ((_fd_new >= 0) && (close(_fd_new) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot close() temporary file descriptor";
                }
                if ((_fd_old >= 0) && (close(_fd_old) < 0)) {
                    ec.assign(errno, std::system_category());
                    msg = "Cannot close() dup()ed file descriptor for original standard output";
                }
            } else {
                ec.assign(ENOSYS, std::system_category());
            }
            if (ec) {
                throw std::system_error{ec, msg};
            }
        }

        static void flush_stdout(std::error_code& ec) noexcept
        {
            const auto mask = std::cout.exceptions();
            std::cout.exceptions(std::ostream::goodbit);
            if (std::cout && !std::cout.flush()) {
                ec.assign(EIO, std::system_category());
            }
            std::cout.exceptions(mask);
            if (stdout && (std::fflush(stdout) < 0)) {
                ec.assign(errno, std::system_category());
            }
        }

        stdout_isatty_guard(const stdout_isatty_guard&) = delete;
        stdout_isatty_guard& operator=(const stdout_isatty_guard&) = delete;

    private:

        int _fd_old{-1};
        int _fd_new{-1};

    };  // struct stdout_isatty_guard

    MSC_AUTO_TEST_CASE(guess_terminal_width_fallback)
    {
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        MSC_SKIP_UNLESS(stdout_isatty_guard::can_be_used());
        const auto guard = stdout_isatty_guard{false};
        MSC_REQUIRE(!stdout_isatty());
        auto eg = msc::test::envguard{"COLUMNS"};
        eg.unset();
        MSC_REQUIRE_EQ(80, msc::guess_terminal_width());
        MSC_REQUIRE_EQ(42, msc::guess_terminal_width(42));
        MSC_REQUIRE_EQ(-1, msc::guess_terminal_width(-1));
    }

    MSC_AUTO_TEST_CASE(guess_terminal_width_environment)
    {
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        MSC_SKIP_UNLESS(stdout_isatty_guard::can_be_used());
        const auto guard = stdout_isatty_guard{false};
        MSC_REQUIRE(!stdout_isatty());
        auto eg = msc::test::envguard{"COLUMNS"};
        eg.set("42");
        MSC_REQUIRE_EQ(42, msc::guess_terminal_width());
        MSC_REQUIRE_EQ(42, msc::guess_terminal_width(100));
    }

    MSC_AUTO_TEST_CASE(guess_terminal_width_environment_bad)
    {
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        MSC_SKIP_UNLESS(stdout_isatty_guard::can_be_used());
        const auto guard = stdout_isatty_guard{false};
        MSC_REQUIRE(!stdout_isatty());
        auto eg = msc::test::envguard{"COLUMNS"};
        for (const auto nonsense : {"", "\t", "0", "-13", "+1", "1a", "0xDEADBEEF", "seventy"}) {
            eg.set(nonsense);
            MSC_REQUIRE_EQ(123, msc::guess_terminal_width(123));
        }
    }

    MSC_AUTO_TEST_CASE(guess_terminal_width_kernel)
    {
        MSC_SKIP_UNLESS(msc::test::envguard::can_be_used());
        MSC_SKIP_UNLESS(stdout_isatty_guard::can_be_used());
        const auto guard = stdout_isatty_guard{true};
        MSC_REQUIRE(stdout_isatty());
        const auto badvalue_environment = 7321;  // Unlikely values which we don' want to see
        const auto badvalue_fallback    = 7331;  // as kernel's answers for the terminal width.
        auto eg = msc::test::envguard{"COLUMNS"};
        eg.set(std::to_string(badvalue_environment));
        const auto answer = msc::guess_terminal_width(badvalue_fallback);
        MSC_REQUIRE_NE(answer, badvalue_environment);
        MSC_REQUIRE_NE(answer, badvalue_fallback);
    }

    void use_iostreams(std::istream& input, std::ostream& output)
    {
        auto xlii = 0;
        input >> xlii;
        output << xlii;
    }

    MSC_AUTO_TEST_CASE(check_stdio_both_good)
    {
        auto input = std::istringstream{"42"};
        auto output = std::ostringstream{};
        use_iostreams(input, output);
        msc::check_stdio(input, output);
    }

    MSC_AUTO_TEST_CASE(check_stdio_in_bad)
    {
        MSC_SKIP("I cannot figure out how to do / test this correctly");
        auto input = std::ifstream{};
        auto output = std::ostringstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EXCEPTION(std::system_error, msc::check_stdio(input, output));
    }

    MSC_AUTO_TEST_CASE(check_stdio_out_bad)
    {
        auto input = std::istringstream{"42"};
        auto output = std::ofstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EXCEPTION(std::system_error, msc::check_stdio(input, output));
    }

    MSC_AUTO_TEST_CASE(check_stdio_both_bad)
    {
        auto input = std::ifstream{};
        auto output = std::fstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EXCEPTION(std::system_error, msc::check_stdio(input, output));
    }

    MSC_AUTO_TEST_CASE(check_stdio_nothrow_good)
    {
        using namespace std::string_literals;
        auto message = "hello, world"s;
        auto input = std::istringstream{"42"};
        auto output = std::ostringstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EQ(true, msc::check_stdio(input, output, message));
        MSC_REQUIRE_EQ("hello, world"s, message);
    }

    MSC_AUTO_TEST_CASE(check_stdio_nothrow_in_bad)
    {
        MSC_SKIP("I cannot figure out how to do / test this correctly");
        auto message = std::string{};
        auto input = std::ifstream{};
        auto output = std::ostringstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EQ(false, msc::check_stdio(input, output, message));
        MSC_REQUIRE_MATCH("(.*: )?Cannot read from standard input(: .*)?"s, message);
    }

    MSC_AUTO_TEST_CASE(check_stdio_nothrow_out_bad)
    {
        auto message = std::string{};
        auto input = std::istringstream{"42"};
        auto output = std::ofstream{};
        use_iostreams(input, output);
        MSC_REQUIRE_EQ(false, msc::check_stdio(input, output, message));
        MSC_REQUIRE_MATCH("(.*: )?Cannot write to standard output(: .*)?"s, message);
    }

    struct mock_application
    {
        std::unique_ptr<int> tally{};
        struct { /* empty */ } parameters{};
        void operator()() { *(this->tally) += 1; };
    };

    MSC_AUTO_TEST_CASE(cli_minimal_1st)
    {
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<mock_application>{"demo", mock_application{std::make_unique<int>()}};
        const char *const argv[] = { __FILE__, nullptr };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(1, *(app->tally));
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
    }

    MSC_AUTO_TEST_CASE(cli_minimal_2nd)
    {
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<mock_application>{"demo", mock_application{std::make_unique<int>()}};
        const char *const argv[] = { __FILE__, "--help", nullptr };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(0, *(app->tally));
        MSC_REQUIRE_NE(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
    }

    MSC_AUTO_TEST_CASE(cli_minimal_3rd)
    {
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<mock_application>{"demo", mock_application{std::make_unique<int>()}};
        const char *const argv[] = { __FILE__, "--version", nullptr };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(0, *(app->tally));
        MSC_REQUIRE_NE(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
    }

    MSC_AUTO_TEST_CASE(cli_minimal_4th)
    {
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<mock_application>{"demo", mock_application{std::make_unique<int>()}};
        const char *const argv[] = { __FILE__, "--more", "--coffee", "--please", nullptr };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_FAILURE, status);
        MSC_REQUIRE_EQ(0, *(app->tally));
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_NE(std::string{}, guard.get_stderr());
    }

    struct maxapp
    {
        struct
        {
            msc::input_file input{"-"};
            msc::output_file output{"-"};
            msc::output_file meta{""};
            msc::fileformats format{};
            bool layout{};
            bool simplify{};
            int nodes{'N'};
            int torus{42};
            int hyperdim{5};
            bool symmetric{};
            msc::algorithms algorithm{};
            msc::distributions distribution{};
            msc::projections projection{};
            std::vector<double> rate{};
            msc::kernels kernel{};
            std::vector<double> width{};
            std::vector<int> bins{};
            std::optional<int> points{};
            int component{};
            //msc::point2d major{};
            //msc::point2d minor{};
            std::vector<double> vicinity{};
            ogdf::Color node_color{};
            ogdf::Color edge_color{};
            ogdf::Color axis_color{};
        } parameters{};
        void operator()() { /* empty */ }
    };

    MSC_AUTO_TEST_CASE(cli_maximal_help)
    {
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<maxapp>{"demo"};
        const char *const argv[] = { __FILE__, "--help", nullptr };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_NE(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
    }

    MSC_AUTO_TEST_CASE(cli_maximal_default)
    {
        using namespace std::string_literals;
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<maxapp>{"demo"};
        app->parameters.format = msc::all_fileformats().front();
        app->parameters.algorithm = msc::all_algorithms().front();
        app->parameters.distribution = msc::all_distributions().front();
        app->parameters.projection = msc::all_projections().front();
        app->parameters.kernel = msc::all_kernels().front();
        const char *const argv[] = {__FILE__, nullptr};
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
        MSC_REQUIRE_EQ(msc::output_file::from_stdio(), app->parameters.output);
        MSC_REQUIRE_EQ(msc::output_file::from_null(), app->parameters.meta);
        MSC_REQUIRE_EQ(msc::all_fileformats().front(), app->parameters.format);
        MSC_REQUIRE_EQ(false, app->parameters.layout);
        MSC_REQUIRE_EQ(false, app->parameters.simplify);
        MSC_REQUIRE_EQ(static_cast<int>('N'), app->parameters.nodes);
        MSC_REQUIRE_EQ(42, app->parameters.torus);
        MSC_REQUIRE_EQ(5, app->parameters.hyperdim);
        MSC_REQUIRE_EQ(false, app->parameters.symmetric);
        MSC_REQUIRE_EQ(msc::all_algorithms().front(), app->parameters.algorithm);
        MSC_REQUIRE_EQ(msc::all_distributions().front(), app->parameters.distribution);
        MSC_REQUIRE_EQ(msc::all_projections().front(), app->parameters.projection);
        MSC_REQUIRE(app->parameters.rate.empty());
        MSC_REQUIRE_EQ(msc::all_kernels().front(), app->parameters.kernel);
        MSC_REQUIRE(app->parameters.width.empty());
        MSC_REQUIRE(app->parameters.bins.empty());
        MSC_REQUIRE_EQ(std::nullopt, app->parameters.points);
        MSC_REQUIRE_EQ(0, app->parameters.component);
        MSC_REQUIRE(app->parameters.vicinity.empty());
        MSC_REQUIRE_EQ(ogdf::Color{}, app->parameters.node_color);
        MSC_REQUIRE_EQ(ogdf::Color{}, app->parameters.edge_color);
        MSC_REQUIRE_EQ(ogdf::Color{}, app->parameters.axis_color);
        MSC_REQUIRE_EQ(msc::input_file::from_stdio(), app->parameters.input);
    }

    MSC_AUTO_TEST_CASE(cli_maximal_nodefault)
    {
        using namespace std::string_literals;
        auto app = msc::command_line_interface<maxapp>{"demo"};
        const char *const argv[] = {__FILE__, nullptr};
        const auto guard = msc::test::capture_stdio{};
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_FAILURE, status);
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_NE(std::string{}, guard.get_stderr());
    }

    MSC_AUTO_TEST_CASE(cli_maximal_long)
    {
        using namespace std::string_literals;
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<maxapp>{"demo"};
        const char *const argv[] = {
            __FILE__,
            "--output=data.dat",
            "--meta=meta.json",
            "--format=matrix-market",
            "--layout",
            "--simplify",
            "--nodes=1000",
            "--torus=0",
            "--hyperdim=1",
            "--symmetric",
            "--algorithm=FMMM",
            "--distribution=NORMAL",
            "--projection=ISOMETRIC",
            "--rate=0.083", "--rate=0.038",
            "--kernel=GAUSSIAN",
            "--width=7.500",
            "--width=0.125",
            "--bins=12345",
            "--points=42",
            "--major",
            "--vicinity=0.0",
            "--vicinity=13.5",
            "--vicinity=29.0",
            "--node-color=#75507b",
            "--edge-color=#5c3566",
            "--axis-color=#cc0000",
            "input.xml",
            nullptr,
        };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
        MSC_REQUIRE_EQ(msc::output_file::from_filename("data.dat"), app->parameters.output);
        MSC_REQUIRE_EQ(msc::output_file::from_filename("meta.json"), app->parameters.meta);
        MSC_REQUIRE_EQ(msc::fileformats::matrix_market, app->parameters.format);
        MSC_REQUIRE_EQ(true, app->parameters.layout);
        MSC_REQUIRE_EQ(true, app->parameters.simplify);
        MSC_REQUIRE_EQ(1000, app->parameters.nodes);
        MSC_REQUIRE_EQ(0, app->parameters.torus);
        MSC_REQUIRE_EQ(1, app->parameters.hyperdim);
        MSC_REQUIRE_EQ(true, app->parameters.symmetric);
        MSC_REQUIRE_EQ(msc::algorithms::fmmm, app->parameters.algorithm);
        MSC_REQUIRE_EQ(msc::distributions::normal, app->parameters.distribution);
        MSC_REQUIRE_EQ(msc::projections::isometric, app->parameters.projection);
        MSC_REQUIRE_EQ(2, app->parameters.rate.size());
        MSC_REQUIRE_CLOSE(1.0E-20, 0.083, app->parameters.rate.at(0));
        MSC_REQUIRE_CLOSE(1.0E-20, 0.038, app->parameters.rate.at(1));
        MSC_REQUIRE_EQ(msc::kernels::gaussian, app->parameters.kernel);
        MSC_REQUIRE_EQ(2, app->parameters.width.size());
        MSC_REQUIRE_EQ(7.5, app->parameters.width.at(0));
        MSC_REQUIRE_EQ(0.125, app->parameters.width.at(1));
        MSC_REQUIRE_EQ(1, app->parameters.bins.size());
        MSC_REQUIRE_EQ(12345, app->parameters.bins.at(0));
        MSC_REQUIRE_EQ(42, app->parameters.points.value());
        MSC_REQUIRE_EQ(1, app->parameters.component);
        MSC_REQUIRE_EQ(3, app->parameters.vicinity.size());
        MSC_REQUIRE_EQ(0.0, app->parameters.vicinity.at(0));
        MSC_REQUIRE_EQ(13.5, app->parameters.vicinity.at(1));
        MSC_REQUIRE_EQ(29.0, app->parameters.vicinity.at(2));
        MSC_REQUIRE_EQ((ogdf::Color{0x75, 0x50, 0x7b}), app->parameters.node_color);
        MSC_REQUIRE_EQ((ogdf::Color{0x5c, 0x35, 0x66}), app->parameters.edge_color);
        MSC_REQUIRE_EQ((ogdf::Color{0xcc, 0x00, 0x00}), app->parameters.axis_color);
        MSC_REQUIRE_EQ(msc::input_file::from_filename("input.xml"), app->parameters.input);
    }

    MSC_AUTO_TEST_CASE(cli_maximal_short)
    {
        using namespace std::string_literals;
        const auto guard = msc::test::capture_stdio{};
        auto app = msc::command_line_interface<maxapp>{"demo"};
        app->parameters.node_color = ogdf::Color{ogdf::Color::Name::Aqua};
        app->parameters.edge_color = ogdf::Color{ogdf::Color::Name::Azure};
        app->parameters.axis_color = ogdf::Color{ogdf::Color::Name::Olive};
        const char *const argv[] = {
            __FILE__,
            "-o", "data.dat",
            "-m", "meta.json",
            "-f", "rudy",
            "-l",
            "-y",
            "-n", "1000",
            "-t", "1",
            "-h", "2",
            "-s",
            "-a", "FMMM",
            "-d", "UNIFORM",
            "-j", "ISOMETRIC",
            "-r", "0.2", "-r", "0.3", "-r", "0.3",
            "-k", "GAUSSIAN",
            "-w", "3.141592653589793",
            "-b", "123",
            "-b", "456",
            "-p", "42",
            "-2",
            "-v", "5",
            "input.xml",
            nullptr,
        };
        const auto status = app(cmdlen(argv), argv);
        MSC_REQUIRE_EQ(EXIT_SUCCESS, status);
        MSC_REQUIRE_EQ(std::string{}, guard.get_stdout());
        MSC_REQUIRE_EQ(std::string{}, guard.get_stderr());
        MSC_REQUIRE_EQ(msc::output_file::from_filename("data.dat"), app->parameters.output);
        MSC_REQUIRE_EQ(msc::output_file::from_filename("meta.json"), app->parameters.meta);
        MSC_REQUIRE_EQ(msc::fileformats::rudy, app->parameters.format);
        MSC_REQUIRE_EQ(true, app->parameters.layout);
        MSC_REQUIRE_EQ(true, app->parameters.simplify);
        MSC_REQUIRE_EQ(1000, app->parameters.nodes);
        MSC_REQUIRE_EQ(1, app->parameters.torus);
        MSC_REQUIRE_EQ(2, app->parameters.hyperdim);
        MSC_REQUIRE_EQ(true, app->parameters.symmetric);
        MSC_REQUIRE_EQ(msc::algorithms::fmmm, app->parameters.algorithm);
        MSC_REQUIRE_EQ(msc::distributions::uniform, app->parameters.distribution);
        MSC_REQUIRE_EQ(msc::projections::isometric, app->parameters.projection);
        MSC_REQUIRE_EQ(3, app->parameters.rate.size());
        MSC_REQUIRE_CLOSE(1.0E-20, 0.2, app->parameters.rate.at(0));
        MSC_REQUIRE_CLOSE(1.0E-20, 0.3, app->parameters.rate.at(1));
        MSC_REQUIRE_CLOSE(1.0E-20, 0.3, app->parameters.rate.at(2));
        MSC_REQUIRE_EQ(msc::kernels::gaussian, app->parameters.kernel);
        MSC_REQUIRE_EQ(1, app->parameters.width.size());
        MSC_REQUIRE_CLOSE(1.0E-15, 3.141592653589793, app->parameters.width.at(0));
        MSC_REQUIRE_EQ(2, app->parameters.bins.size());
        MSC_REQUIRE_EQ(123, app->parameters.bins.at(0));
        MSC_REQUIRE_EQ(456, app->parameters.bins.at(1));
        MSC_REQUIRE_EQ(42, app->parameters.points);
        MSC_REQUIRE_EQ(2, app->parameters.component);
        MSC_REQUIRE_EQ(1, app->parameters.vicinity.size());
        MSC_REQUIRE_EQ(5.0, app->parameters.vicinity.at(0));
        MSC_REQUIRE_EQ(ogdf::Color{ogdf::Color::Name::Aqua}, app->parameters.node_color);
        MSC_REQUIRE_EQ(ogdf::Color{ogdf::Color::Name::Azure}, app->parameters.edge_color);
        MSC_REQUIRE_EQ(ogdf::Color{ogdf::Color::Name::Olive}, app->parameters.axis_color);
        MSC_REQUIRE_EQ(msc::input_file::from_filename("input.xml"), app->parameters.input);
    }

}  // namespace /*anonymous*/
