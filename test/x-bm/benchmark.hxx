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

/**
 * @file benchmark.hxx
 *
 * @brief
 *     Utility features for writing micro-benchmarks that are compatible with the runner script.
 *
 */

#ifndef MSC_BENCHMARK_HXX
#define MSC_BENCHMARK_HXX

#include <chrono>
#include <cstddef>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>

namespace msc::benchmark
{

    /**
     * @brief
     *     Introduces a line in the code among which the compiler cannot reorder code.
     *
     * Otherwise, this function is a no-op.
     *
     */
    inline void compiler_barrier() noexcept {
        asm volatile ("" : : : "memory");
    }

    /**
     * @brief
     *     Introduces a line in the code among which the compiler cannot make assumptions about the content of the
     *     memory reacahble via `p`.
     *
     * Otherwise, this function is a no-op.
     *
     * @param p
     *     pointer to memory to clobber
     *
     */
    inline void clobber_memory(const void *const p) noexcept {
        asm volatile ("" : : "rm"(p) : "memory");
    }

    /**
     * @brief
     *     Get a seeded random engine that is ready to use.
     *
     * The current implementation always seeds the engine non-deterministically but it might be changed in the future to
     * honor user options to use an explicit seed value for reproducability.  Benchmarks should always use this function
     * to obtain an engine.
     *
     * @returns
     *     seeded random engine
     *
     */
    std::default_random_engine get_random_engine();

    /** @brief Clock type used for benchmarking. */
    using clock_type = std::chrono::steady_clock;

    /** @brief Duration type used for benchmarking. */
    using duration_type = std::chrono::duration<double>;

    /**
     * @brief
     *     Statistical result of running a benchmark.
     *
     */
    struct result
    {

        /** @brief Average wall-time taken by the code (non-negative). */
        duration_type mean{};

        /** @brief Standard deviation of the wall-time taken (non-negative). */
        duration_type stdev{};

        /** @brief Number of samples used to compute the statistics (at least 3). */
        std::size_t n{};

    };

    /**
     * @brief
     *     Exception used to indicate that a benchmark has failed and no result could be obtained.
     *
     */
    struct failure : std::runtime_error
    {

        /**
         * @brief
         *     Creates a new exception object with the provided message.
         *
         * @param msg
         *     informal explanation why the benchmark failed
         *
         */
        failure(const std::string& msg) : std::runtime_error{msg}
        {
        }

    };

    /**
     * @brief
     *     Contraints on benchmark execution.
     *
     */
    struct constraints
    {

        /** @brief Maximum amount of time (no limit if zero). */
        duration_type timeout{};

        /** @brief Maximum number of repetitions (no limit if zero). */
        std::size_t repetitions{};

        /** @brief Number of samples to throw away at the beginning. */
        std::size_t warmup{};

        /** @brief Fraction of best timing results to use. */
        double quantile{};

        /** @brief Desired relative standard deviation. */
        double significance{};

        /** @brief Whether to produce verbose output. */
        bool verbose{};

    };

    /**
     * @brief
     *     Loads benchmark constraints from the environment.
     *
     * This function checks the following environment variables:
     *
     *  - `BENCHMARK_TIMEOUT` (default: no limit)
     *  - `BENCHMARK_REPETITIONS` (default: no limit)
     *  - `BENCHMARK_WARMUP` (default: 0)
     *  - `BENCHMARK_QUANTILE` (default: 1)
     *  - `BENCHMARK_SIGNIFICANCE` (default: 20 %)
     *  - `BENCHMARK_VERBOSE` (default: no)
     *
     * @returns
     *     `constraints` initialized from environment and defaults
     *
     * @throws boost::program_options::error
     *     if the environment contains bad values
     *
     */
    constraints get_constraints_from_environment();

    /**
     * @brief
     *     Runs a benchmark repetitively until the desired significance is reached or a constraint limit is exceeded,
     *     whatever happens first.
     *
     * If a constraint limit is exceeded before at least three data points could be sampled, an exception is `throw`n.
     *
     * @tparam CallT
     *     callable type that can be invoked with arguments `args`
     *
     * @tparam ArgTs
     *     types of any arguments that should be passed to the benchmark
     *
     * @param c
     *     constraints subject to which the benchmark should be run
     *
     * @param bench
     *     callable object to benchmark
     *
     * @param args
     *     additional arguments to pass to the callable
     *
     * @returns
     *     the statistical result of running the benchmark
     *
     * @throws failure
     *     if a constraint limit is hit before a result could be obtained
     *
     */
    template <typename CallT, typename... ArgTs>
    result run_benchmark(const constraints& c, CallT&& bench, ArgTs&&... args);

    /**
     * @brief
     *     Prints a result to standard output.
     *
     * The output format is
     *
     *     MEAN STDEV N
     *
     * where times are in seconds.  This is meant to be an easily parseable format.
     *
     * @param res
     *     result to print
     *
     * @throws std::invalid_argument
     *     if the values in `res` are garbage
     *
     * @throws std::system_error
     *     if the result could not be written
     *
     */
    void print_result(const result& res);

    /**
     * @brief
     *     Off-the-shelf command-line interface that should be good enough for most micro-benchmarks.
     *
     * Writing good micro-benchmarks is already hard enough.  This `class` aims to free the programmer of the most
     * tedious jobs when also having to provide at least a decent command-line interface.  Unlike fully-fledged
     * solutions like the *Boost Program Options* library, it trades simplicity for generality.  There are exactly two
     * kinds of command-line arguments you can use with this `class`: options that expect a size argument and boolean
     * flags.  This should be enough for most micro-benchmarks, though.
     *
     * Contrary to good software engineering practice, this `class` requires you to call its member functions in a
     * specific order.  This is again for simplicity.  Here is a sample usage.
     *
     *     benchmark_setup setup{"mumble", "Measures the speed of the mumbler component."};
     *     setup.add_cmd_arg("count", "number of mumbles to use");
     *     setup.add_cmd_flag("fuzzy", "use the fuzzy variant of the mumbler");
     *     if (!setup.process(argc, argv)) {
     *         return EXIT_SUCCESS;
     *     }
     *     const std::size_t count = setup.get_cmd_arg("count");
     *     const bool fuzzy = setup.get_cmd_arg("fuzzy");
     *     const constraints cons = setup.get_constraints();
     *
     * The order of member function calls only matters for the things above and below the call to `process`.
     *
     * The `constraints` object will first be initialized from the environment (as if by
     * `get_constraints_from_environment`) and then updated by any command-line options.
     *
     * The following command-line options are always added implicitly and must not be added again.
     *
     *  - `--help` -- show help text and exit
     *  - `--version` -- show dummy version text and exit
     *  - `--verbose` -- overrides the environment variable `BENCHMARK_VERBOSE`
     *  - `--timeout=ARG` -- overrides the environment variable `BENCHMARK_TIMEOUT`
     *  - `--repetitions=ARG` -- overrides the environment variable `BENCHMARK_REPETITIONS`
     *  - `--warmup=ARG` -- overrides the environment variable `BENCHMARK_WARMUP`
     *  - `--quantile=ARG` -- overrides the environment variable `BENCHMARK_QUANTILE`
     *  - `--significance=ARG` -- overrides the environment variable `BENCHMARK_SIGNIFICANCE`
     *
     */
    class benchmark_setup final
    {
    public:

        /**
         * @brief
         *     Creates a new `benchmark_setup`.
         *
         * @param name
         *     name of the benchmark
         *
         * @param description
         *     short description of the benchmark
         *
         */
        benchmark_setup(const std::string& name, const std::string& description);

        /**
         * @brief
         *     Adds a command-line option that takes a string as argument.
         *
         * If called with `name` as `foo` then the program will accept a command-line option `--foo` that expects an
         * arbitrary string.  After `process` has been called and `return`ed `true`, the value specified by the user
         * will be available via `get_cmd_arg("foo")`.
         *
         * Command-line arguments added this way are optional.
         *
         * Adding a command-line argument that clashes with an already added one is an error.  Note that some options
         * are always defined.
         *
         * @param name
         *     name of the command-line option to add
         *
         * @param description
         *     short description of the command-line option
         *
         * @param fallback
         *     default value if the user does not specify the option
         *
         * @throws std::invalid_argument
         *     if `name` clashes with an existing parameter
         *
         */
        void add_cmd(const std::string& name, const std::string& description, const std::string& fallback = "");

        /**
         * @brief
         *     Adds a command-line option that expects non-negative integer as argument.
         *
         * If called with `name` as `foo` then the program will accept a command-line option `--foo` that expects a
         * non-negative integer.  After `process` has been called and `return`ed `true`, the value specified by the user
         * will be available via `get_cmd_arg("foo")`.
         *
         * Command-line arguments added this way are always mandatory.
         *
         * Adding a command-line argument that clashes with an already added one is an error.  Note that some options
         * are always defined.
         *
         * @param name
         *     name of the command-line option to add
         *
         * @param description
         *     short description of the command-line option
         *
         * @throws std::invalid_argument
         *     if `name` clashes with an existing parameter
         *
         */
        void add_cmd_arg(const std::string& name, const std::string& description);

        /**
         * @brief
         *     Adds a command-line option that acts as a boolean flag.
         *
         * If called with `name` as `foo` then the program will accept a command-line option `--foo` that expects no
         * arguments.  After `process` has been called and `return`ed `true`, `get_cmd_arg("foo")` will `return` `true`
         * if and only if `--foo` was seen on the command-line.
         *
         * Adding a command-line argument that clashes with an already added one is an error.  Note that some options
         * are always defined.
         *
         * @param name
         *     name of the command-line option to add
         *
         * @param description
         *     short description of the command-line option
         *
         * @throws std::invalid_argument
         *     if `name` clashes with an existing parameter
         *
         */
        void add_cmd_flag(const std::string& name, const std::string& description);

        /**
         * @brief
         *     Parses the command-line arguments.
         *
         * If the `--help` or the `--version` option is seen, the appropriate action will be performed and `false` will
         * be `return`ed.  This indicates to the calling application that it should quit immediately with a status
         * indicating success without performing the benchmark.  Otherwise, if the command-line arguments were not
         * valid, a `boost::program_options::error` will be `throw`n and the application should quit immediately with a
         * status indicating failure.  Other exceptions might be `throw`n as well, for example, if a memory exhaustion
         * is encountered or if standard output is not writeable.  All these excaptions will inherit from
         * `std::exception`.
         *
         * @param argc
         *     number of elements in the `argv` array
         *
         * @param argv
         *     array of command-line arguments
         *
         * @returns
         *     whether the program should run the actual benchmark
         *
         * @throws boost::program_options::error
         *     if the command-line was invalid
         *
         */
        bool process(const int argc, const char *const *const argv);

        /**
         * @brief
         *     Obtains the value for the option `name` that was provided by the user.
         *
         * @param name
         *     name of the command-line argument
         *
         * @returns
         *     value of the argument as provided by the user
         *
         * @throws std::invalid_argument
         *     if `name` was not registred before calling `process`
         *
         */
        std::string get_cmd(const std::string& name) const;

        /**
         * @brief
         *     Obtains the value for the option `name` that was provided by the user.
         *
         * @param name
         *     name of the command-line argument
         *
         * @returns
         *     value of the argument as provided by the user
         *
         * @throws std::invalid_argument
         *     if `name` was not registred before calling `process`
         *
         */
        std::size_t get_cmd_arg(const std::string& name) const;

        /**
         * @brief
         *     Tells whether the user provided the flag `name`.
         *
         * @param name
         *     name of the command-line flag
         *
         * @returns
         *     whether the user provided the flag `name`
         *
         * @throws std::invalid_argument
         *     if `name` was not registred before calling `process`
         *
         */
        bool get_cmd_flag(const std::string& name) const;

        /**
         * @brief
         *     `return`s the constraints for running this benchmark.
         *
         * The object will be initialized from the environment and additionally by command-line arguments as explained
         * in the `class`-level documentation of this `class`.
         *
         * @returns
         *     reference to the initialized `constraints` object
         *
         */
        const constraints& get_constraints() const noexcept;

    private:

        /** @brief Name of the benchmark. */
        std::string _name{};

        /** @brief Description of the benchmark. */
        std::string _description{};

        /** @brief Registred command-line arguments and their descriptions. */
        std::map<std::string, std::string> _cmd_help{};

        /** @brief Values of textual command-line arguments after argument processing. */
        std::map<std::string, std::string> _cmd_vals{};

        /** @brief Values of integral command-line arguments after argument processing. */
        std::map<std::string, std::ptrdiff_t> _cmd_vals_args{};

        /** @brief Values of boolean command-line arguments after argument processing. */
        std::map<std::string, bool> _cmd_vals_flags{};

        /** @brief Constraints for this benchmark. */
        constraints _constraints{};

    };  // class benchmark_setup

}  // namespace msc::benchmark

#define MSC_INCLUDED_FROM_BENCHMARK_HXX
#include "benchmark.txx"
#undef MSC_INCLUDED_FROM_BENCHMARK_HXX

#endif  // !defined(MSC_BENCHMARK_HXX)
