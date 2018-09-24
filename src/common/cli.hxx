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
 * @file cli.hxx
 *
 * @brief
 *     Common facilities for building CLIs.
 *
 */

#ifndef MSC_CLI_HXX
#define MSC_CLI_HXX

#include <cstddef>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>

#include "enums/kernels.hxx"
#include "file.hxx"

namespace msc
{

    /** @brief Enumeration type that is used in the CLI.  */
    enum class stress_modi
    {
        fixed       = 0,  ///< use fixed scale and default node distance
        fit_nodesep = 1,  ///< fit quadratic parabola against node distance and report minimum
        fit_scale   = 2,  ///< fit quadratic parabola against scale using default node distance and report minimum
    };

    /**
     * @brief
     *     Base-class for command-line interfaces.
     *
     * All strings must not contain vertical white-space and should only contain simple text.
     *
     */
    struct cli_base
    {
        /** @brief Informal name of the program.  */
        std::string prog{};

        /** @brief Program invocation synopsis.  */
        std::string usage{};

        /** @brief Zero or more paragraphs of general information about the program.  */
        std::vector<std::string> help{};

        /** @brief Zero or more paragraphs of final remarks about the program.  */
        std::vector<std::string> epilog{};

        /** @brief Description of influential environment variables.  */
        std::map<std::string, std::string> environ{};
    };

    /**
     * @brief
     *     Generic command-line interface.
     *
     * In order to build a CLI using this facility, you need to define a `struct` that has (at least) all well-known
     * command-line parameters as members.  The following members will be understood.  The `struct` can be named
     * anything but let's call it `ParamsT` for now.
     *
     * <table>
     *   <tr>
     *     <th>flag</th>
     *     <th>option</th>
     *     <th>member</th>
     *     <th>type</th>
     *     <th>default</th>
     *     <th>comment</th>
     *   </tr>
     *   <tr>
     *     <td>&nbsp;</td>
     *     <td></td>
     *     <td>`input`</td>
     *     <td>`std::string`</td>
     *     <td>`"-"`</td>
     *     <td>optional positional argument</td>
     *   </tr>
     *   <tr>
     *     <td>&nbsp;</td>
     *     <td></td>
     *     <td>`input1st`</td>
     *     <td>`input_file`</td>
     *     <td>`"-"`</td>
     *     <td>mandatory positional argument</td>
     *   </tr>
     *   <tr>
     *     <td>&nbsp;</td>
     *     <td></td>
     *     <td>`input2nd`</td>
     *     <td>`input_file`</td>
     *     <td>`"-"`</td>
     *     <td>mandatory positional argument</td>
     *   </tr>
     *   <tr>
     *     <td>`-o`</td>
     *     <td>`--output`</td>
     *     <td>`output`</td>
     *     <td>`output_file`</td>
     *     <td>`"-"`</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--output-layout`</td>
     *     <td>`output_layout`</td>
     *     <td>`output_file`</td>
     *     <td></td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-m`</td>
     *     <td>`--meta`</td>
     *     <td>`meta`</td>
     *     <td>`output_file`</td>
     *     <td>`""`</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-f`</td>
     *     <td>`--format`</td>
     *     <td>`format`</td>
     *     <td>`#fileformats`</td>
     *     <td><em>see comment</em></td>
     *     <td>mandatory if initialized to zero, otherwise optional with that default</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--list-formats`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added together with `--format`</td>
     *   </tr>
     *   <tr>
     *     <td>`-l`</td>
     *     <td>`--layout`</td>
     *     <td>`layout`</td>
     *     <td>`bool`</td>
     *     <td>`false`</td>
     *     <td>boolean flag</td>
     *   </tr>
     *   <tr>
     *     <td>`-l`</td>
     *     <td>`--layout`</td>
     *     <td>`layout`</td>
     *     <td>`std::optional&lt;bool&gt;`</td>
     *     <td>`std::nullopt`</td>
     *     <td>tristate flag</td>
     *   </tr>
     *   <tr>
     *     <td>`-y`</td>
     *     <td>`--simplify`</td>
     *     <td>`simplify`</td>
     *     <td>`bool`</td>
     *     <td>`false`</td>
     *     <td>boolean flag</td>
     *   </tr>
     *   <tr>
     *     <td>`-n`</td>
     *     <td>`--nodes`</td>
     *     <td>`nodes`</td>
     *     <td>`int`</td>
     *     <td>0 &lt; <var>N</var> &le; `INT_MAX`</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-t`</td>
     *     <td>`--torus`</td>
     *     <td>`torus`</td>
     *     <td>`int`</td>
     *     <td><var>N</var> &ge; 0</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-h`</td>
     *     <td>`--hyperdim`</td>
     *     <td>`hyperdim`</td>
     *     <td>`int`</td>
     *     <td><var>N</var> &gt; 0</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-s`</td>
     *     <td>`--symmetric`</td>
     *     <td>`symmetric`</td>
     *     <td>`bool`</td>
     *     <td>`false`</td>
     *     <td>boolean flag</td>
     *   </tr>
     *   <tr>
     *     <td>`-a`</td>
     *     <td>`--algorithm`</td>
     *     <td>`algorithm`</td>
     *     <td>`#algorithms`</td>
     *     <td><em>see comment</em></td>
     *     <td>mandatory if initialized to zero, otherwise optional with that default</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--list-algorithms`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added together with `--algorithm`</td>
     *   </tr>
     *   <tr>
     *     <td>`-d`</td>
     *     <td>`--distribution`</td>
     *     <td>`distribution`</td>
     *     <td>`#distributions`</td>
     *     <td><em>see comment</em></td>
     *     <td>mandatory if initialized to zero, otherwise optional with that default</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--list-distributions`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added together with `--distribution`</td>
     *   </tr>
     *   <tr>
     *     <td>`-j`</td>
     *     <td>`--projection`</td>
     *     <td>`projection`</td>
     *     <td>`#projections`</td>
     *     <td><em>see comment</em></td>
     *     <td>mandatory if initialized to zero, otherwise optional with that default</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--list-projections`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added together with `--projection`</td>
     *   </tr>
     *   <tr>
     *     <td>`-r`</td>
     *     <td>`--rate`</td>
     *     <td>`rate`</td>
     *     <td>`std::vector<double>`</td>
     *     <td>empty</td>
     *     <td>zero or more values in the unit interval</td>
     *   </tr>
     *   <tr>
     *     <td>`-k`</td>
     *     <td>`--kernel`</td>
     *     <td>`kernel`</td>
     *     <td>`#kernels`</td>
     *     <td><em>see comment</em></td>
     *     <td>mandatory if initialized to zero, otherwise optional with that default</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--list-kernels`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added together with `--kernel`</td>
     *   </tr>
     *   <tr>
     *     <td>`-w`</td>
     *     <td>`--width`</td>
     *     <td>`width`</td>
     *     <td>`std::vector&lt;double&gt;`</td>
     *     <td>empty</td>
     *     <td>may be passed zero or more times</td>
     *   </tr>
     *   <tr>
     *     <td>`-b`</td>
     *     <td>`--bins`</td>
     *     <td>`bins`</td>
     *     <td>`std::vector&lt;int&gt;`</td>
     *     <td>empty</td>
     *     <td>may be passed zero or more times</td>
     *   </tr>
     *   <tr>
     *     <td>`-p`</td>
     *     <td>`--points`</td>
     *     <td>`points`</td>
     *     <td>`std::optional&lt;int&gt;`</td>
     *     <td>`std::nullopt`</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td>`-1`</td>
     *     <td>`--major`</td>
     *     <td>`component`</td>
     *     <td>`int`</td>
     *     <td>`0`</td>
     *     <td>stores `1` in `component` when passed</td>
     *   </tr>
     *   <tr>
     *     <td>`-2`</td>
     *     <td>`--minor`</td>
     *     <td>`component`</td>
     *     <td>`int`</td>
     *     <td>`0`</td>
     *     <td>stores `2` in `component` when passed</td>
     *   </tr>
     *   <tr>
     *     <td>`-v`</td>
     *     <td>`--vicinity`</td>
     *     <td>`vicinity`</td>
     *     <td>`std::vector<double>`</td>
     *     <td>empty</td>
     *     <td>zero or more non-negative real values</td>
     *   </tr>
     *   <tr>
     *     <td>`-c`</td>
     *     <td>`--clever`</td>
     *     <td>`clever`</td>
     *     <td>`bool`</td>
     *     <td>`false`</td>
     *     <td>boolean flag</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--fit-nodesep`</td>
     *     <td>`stress_modus`</td>
     *     <td>`msc::stress_modi`</td>
     *     <td>`msc::stress_modi::fixed`</td>
     *     <td>boolean flag, always added together with `--fit-scale`</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--fit-scale`</td>
     *     <td>`stress_modus`</td>
     *     <td>`msc::stress_modi`</td>
     *     <td>`msc::stress_modi::fixed`</td>
     *     <td>boolean flag, always added together with `--fit-nodesep`</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--node-color`</td>
     *     <td>`node_color`</td>
     *     <td>`ogdf::Color`</td>
     *     <td>anything</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--edge-color`</td>
     *     <td>`edge_color`</td>
     *     <td>`ogdf::Color`</td>
     *     <td>anything</td>
     *     <td>optional</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--tikz`</td>
     *     <td>`tikz`</td>
     *     <td>`bool`</td>
     *     <td>`false`</td>
     *     <td>boolean flag</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--help`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added</td>
     *   </tr>
     *   <tr>
     *     <td></td>
     *     <td>`--version`</td>
     *     <td></td>
     *     <td></td>
     *     <td></td>
     *     <td>always added</td>
     *   </tr>
     * </table>
     *
     * Next, you have to write a `struct` (let's call it `AppT`) with a member of type `ParamsT` that must be named
     * `parameters`.  Furthermore, the `struct` must have its call operator overloaded to take no arguments and return
     * `void`.  (Errors should be reported via exceptions.)
     *
     * @tparam AppT
     *     see above
     *
     */
    template <typename AppT>
    class command_line_interface final : public cli_base
    {

        static_assert(
            std::is_same_v<void, decltype(std::declval<AppT&>()())>,
            "AppT must have an overloaded call operator that takes no arguments and returns `void`.  Errors have to be"
            " reported via exceptions."
        );

        static_assert(
            std::is_class_v<decltype(AppT::parameters)>,
            "AppT must define a member `parameters` for holding the parsed command-line parameters of the application."
        );

    public:

        /**
         * @brief
         *     Creates a new command-line application.
         *
         * @param prog
         *     informal program name
         *
         * @param app
         *     application to run internally
         *
         */
        explicit command_line_interface(std::string prog, AppT app = AppT{});

        /**
         * @brief
         *     Runs the main function `AppT::operator()` in a `try` / `catch` block after command-line parsing and quite
         *     a bit more.
         *
         * Here is how this function proceeds:
         *
         * 1. Parse the given command-line arguments and use them to initialize the members of `AppT::parameters` as
         *    described above.  If an option such as `--help` is encountered, a sythesized text using the attributes
         *    from the `cli_base` base (which therefore should be initialized) is shown and the following steps are
         *    skipped.
         *
         * 2. Call `set_resource_limits()`.  This is done after command-line parsing so that if there is an error
         *    setting the resource limits, users can at least view the help message (and therefore maybe figure out what
         *    to change to avoid the error).  It is highly unlikely that command-line parsing would exceed any
         *    reasonable limit.
         *
         * 3. Invoke `AppT::operator()`.
         *
         * 4. If step (1) through (3) raised any exceptions, handle them by printing an appropriate error message.
         *
         * 5. Test if standard input (`std::cin`) or standard output (`std::cout`)  are okay.
         *
         * 6. Return `EXIT_FAILURE` if an invalid command-line was passed, an exception thrown or the check in (5)
         *    failed and `EXIT_SUCCESS` otherwise.
         *
         * Any I/O done by the internal application nonwithstanding , all I/O generated by this class directly will go
         * to the standard C++ streams `std::cout` and `std::cerr`.  Low-level C or POSIX I/O will not be used.
         *
         * @param argc
         *     number of arguments as specified by the C standard for `main`'s first argument
         *
         * @param argv
         *     argument vector as specified by the C standard for `main`'s second argument
         *
         * @returns
         *     exit status for the application
         *
         */
        [[nodiscard]] int operator()(const int argc, const char *const *const argv);

        /**
         * @brief
         *     Returns a pointer to the internal application instance.
         *
         * @note
         *     The sole reason for this function is unit testing.
         *
         * @returns
         *     pointer to internal application
         *
         */
        AppT* operator->() noexcept
        {
            return &_app;
        }

    private:

        /** @brief The internal application.  */
        AppT _app{};

    };  // class command_line_interface

    /**
     * @brief
     *     Guesses the width of the output terminal.
     *
     * If the environment variable `COLUMNS` is set and can be parsed as an integer with a value in the interval
     * `(0, INT_MAX]`, that value is returned.  Otherwise, `fallback` is returned.
     *
     * @param fallback
     *     value to return as a last resort
     *
     * @returns
     *     guestimated terminal width in columns
     *
     */
    int guess_terminal_width(int fallback = 80) noexcept;

    /**
     * @brief
     *     Checks whether standard input or output are in an error state.
     *
     * EOF is not considered an error state.
     *
     * The intended use of this function is to be called at the end of `main` as `check_stdio(std::cin, std::cout)` to
     * make sure I/O errors are not silently passed over.
     *
     * @param input
     *     standard input stream
     *
     * @param output
     *     standard output stream
     *
     * @throws std::system_error
     *     if either stream is not okay
     *
     */
    void check_stdio(std::istream& input, std::ostream& output);

    /**
     * @brief
     *     Checks whether standard input or output are in an error state.
     *
     * This function performs the same check as the throwing overload but instead of throwing an exception returns
     * whether the streams were both okay and, if they are not, provides an appropriate error message.
     *
     * In the rare event that assigning to the string required a memory reallocation that fails, `std::terminate` will
     * be called.  If this is a concern, make sure to `reserve()` sufficient space in the string beforehand.
     *
     * @param input
     *     standard input stream
     *
     * @param output
     *     standard output stream
     *
     * @param message
     *     if `false` is returned, an error message is assigned to the string, otherwise, it is left unchanged
     *
     * @returns
     *     whether both streams are okay
     *
     */
    bool check_stdio(std::istream& input, std::ostream& output, std::string& message) noexcept;

    /**
     * @brief
     *     Provides a generic message that informs the user about the expansion of `%` characters in file names.
     *
     * @returns
     *     generic help text
     *
     */
    const std::string& helptext_file_name_expansion();

    /** @brief Convenience type intended as base class for layout interpolation CLI parameters.  */
    struct cli_parameters_interpolation
    {
        /** @brief First input layout file.  */
        input_file input1st{};

        /** @brief Second input layout file.  */
        input_file input2nd{};

        /** @brief Output layout file (pattern).  */
        output_file output{"-"};

        /** @brief Meta data output file.  */
        output_file meta{};

        /** @brief Whether or not to be clever.  */
        bool clever{};

        /** @brief List of interpolation rates.  */
        std::vector<double> rate{};

        /**
         * @brief
         *     Convenience member function for obtaining the file name for the specified rate.
         *
         * The behavior is undefined unless `0 <= rate <= 1`.
         *
         * @param rate
         *     interpolation rate
         *
         * @returns
         *     output file name
         *
         */
        output_file expand_filename(double rate) const;

    };  // struct cli_parameters_interpolation

    /** @brief Convenience type intended as base class for layout worsening CLI parameters.  */
    struct cli_parameters_worsening
    {

        /** @brief Input layout file.  */
        input_file input{"-"};

        /** @brief Output layout file.  */
        output_file output{"-"};

        /** @brief Meta data output file.  */
        output_file meta{};

        /** @brief List of worsening rates.  */
        std::vector<double> rate{};

        /**
         * @brief
         *     Convenience member function for obtaining the file name for the specified rate.
         *
         * The behavior is undefined unless `0 <= rate <= 1`.
         *
         * @param rate
         *     worsening rate
         *
         * @returns
         *     output file name
         *
         */
        output_file expand_filename(double rate) const;

    };  // struct cli_parameters_worsening

    /** @brief Convenience type intended as base class for property computation CLI parameters.  */
    struct cli_parameters_property
    {

        /** @brief Input layout file.  */
        input_file input{"-"};

        /** @brief Output data file.  */
        output_file output{"-"};

        /** @brief Meta data output file.  */
        output_file meta{};

        /** @brief Kernel to use for analysis.  */
        msc::kernels kernel{};

        /** @brief List of bin / filter widths.  */
        std::vector<double> width{};

        /** @brief List of histogram bin counts.  */
        std::vector<int> bins{};

        /** @brief List of numbers of evaluation points.  */
        std::optional<int> points{};

        /**
         * @brief
         *     Returns the number of iterations to be performed.
         *
         * @returns
         *     `max(1, width.size(), bins.size())`
         *
         */
        std::size_t iterations() const noexcept;

    };  // struct cli_parameters_property

    /** @brief Convenience type intended as base class for localized property computation CLI parameters.  */
    struct cli_parameters_property_local : cli_parameters_property
    {

        /** @brief List of vicinities.  */
        std::vector<double> vicinity{};

    };  // struct cli_parameters_property_local

    struct cli_parameters_metric
    {
        input_file input{"-"};
        //output_file output{"-"};
        output_file meta{};
    };

    /**
     * @brief
     *     Constructs a file name by replacing each `%` in `pattern.filename()` by a string representation of
     *     `iteration`.
     *
     * This function returns a verbatim copy of `pattern` unless `pattern.terminal() == terminals::file`.  The terminal
     * and compression attributes are preserved in every case.
     *
     * @param pattern
     *     file name pattern (including zero or more `%` characters)
     *
     * @param iteration
     *     counter to substitute
     *
     * @returns
     *     expanded file name
     *
     */
    output_file expand_filename(const output_file& pattern, std::size_t iteration);

    /**
     * @brief
     *     Constructs a file name by replacing the first and second `%` in `pattern.filename()` by a string
     *     representation of `major` and `minor` respectively.
     *
     * This function returns a verbatim copy of `pattern` unless `pattern.terminal() == terminals::file`.  The terminal
     * and compression attributes are preserved in every case.
     *
     * @param pattern
     *     file name pattern (including zero, one or two `%` characters)
     *
     * @param major
     *     first counter to substitute
     *
     * @param minor
     *     second counter to substitute
     *
     * @returns
     *     expanded file name
     *
     * @throws std::invalid_argument
     *     if the file name contains more than three `%` characters
     *
     */
    output_file expand_filename(const output_file& pattern, std::size_t major, std::size_t minor);

}  // namespace msc

#define MSC_INCLUDED_FROM_CLI_HXX
#include "cli.txx"
#undef MSC_INCLUDED_FROM_CLI_HXX

#endif  // !defined(MSC_CLI_HXX)
