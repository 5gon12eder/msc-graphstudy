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

#ifndef MSC_INCLUDED_FROM_CLI_HXX
#  error "Never `#include <cli.txx>` directly, `#include <cli.hxx>` instead"
#endif

#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <ogdf/basic/graphics.h>

#include "enums/algorithms.hxx"
#include "enums/distributions.hxx"
#include "enums/fileformats.hxx"
#include "enums/kernels.hxx"
#include "enums/projections.hxx"
#include "point.hxx"
#include "strings.hxx"

namespace msc
{

    namespace detail::cli
    {

        struct system_exit : std::exception
        {
            int status{};
        };

        namespace po = boost::program_options;

        template <template<typename...> typename... HandlerTs>
        struct argument_handler
        {

            template <typename CliResT>
            static void add(CliResT& results,
                            po::options_description& description,
                            po::positional_options_description& positional)
            {
                return (HandlerTs<CliResT>::add(results, description, positional), ...);
            }

            template <typename CliResT>
            static void handle_before(CliResT& results, po::variables_map& varmap)
            {
                return (HandlerTs<CliResT>::handle_before(results, varmap), ...);
            }

            template <typename CliResT>
            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                return (HandlerTs<CliResT>::handle_after(results, varmap), ...);
            }

        };  // struct argument_handler

        template <template<typename...> typename... HandlerTs>
        struct option_handler
        {

            template <typename CliResT>
            static void add(CliResT& results, po::options_description& description)
            {
                return (HandlerTs<CliResT>::add(results, description), ...);
            }

            template <typename CliResT>
            static void handle_before(CliResT& results, po::variables_map& varmap)
            {
                return (HandlerTs<CliResT>::handle_before(results, varmap), ...);
            }

            template <typename CliResT>
            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                return (HandlerTs<CliResT>::handle_after(results, varmap), ...);
            }

        };  // struct option_handler

        template <typename CliResT>
        struct basic_argument_handler
        {
            static void add(CliResT&, po::options_description&, po::positional_options_description&) noexcept { }
            static void handle_before(CliResT&, po::variables_map&) noexcept { }
            static void handle_after(CliResT&, po::variables_map&) noexcept { }
        };

        template <typename CliResT>
        struct basic_option_handler
        {
            static void add(CliResT&, po::options_description&) noexcept { }
            static void handle_before(CliResT&, po::variables_map&) noexcept { }
            static void handle_after(CliResT&, po::variables_map&) noexcept { }
        };

        template <typename CliResT, typename = void>
        struct argument_input : basic_argument_handler<CliResT> { };

        template <typename CliResT>
        struct argument_input<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::input), input_file>>>
            : basic_argument_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results,
                            po::options_description& description,
                            po::positional_options_description& positional)
            {
                assert(results.input.terminal() == terminals::stdio);
                positional.add("input", 1);
                description.add_options()(
                    "input", po::value<std::string>()->value_name("FILE"),
                    "read input data from FILE (default: read from standard input)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("input")) {
                    results.input.assign_from_spec(varmap["input"].as<std::string>());
                }
            }

        };  // struct argument_input

        template <typename CliResT, typename = void>
        struct argument_input_1st : basic_argument_handler<CliResT> { };

        template <typename CliResT>
        struct argument_input_1st<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::input1st), input_file>>>
            : basic_argument_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results,
                            po::options_description& description,
                            po::positional_options_description& positional)
            {
                assert(results.input1st.terminal() == terminals::null);
                positional.add("input1st", 1);
                description.add_options()(
                    "input1st", po::value<std::string>()->value_name("FILE")->required(),
                    "read first input data from FILE (required)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                results.input1st.assign_from_spec(varmap["input1st"].as<std::string>());
            }

        };  // struct argument_input_1st

        template <typename CliResT, typename = void>
        struct argument_input_2nd : basic_argument_handler<CliResT> { };

        template <typename CliResT>
        struct argument_input_2nd<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::input2nd), input_file>>>
            : basic_argument_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results,
                            po::options_description& description,
                            po::positional_options_description& positional)
            {
                assert(results.input2nd.terminal() == terminals::null);
                positional.add("input2nd", 1);
                description.add_options()(
                    "input2nd", po::value<std::string>()->value_name("FILE")->required(),
                    "read second input data from FILE (required)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                results.input2nd.assign_from_spec(varmap["input2nd"].as<std::string>());
            }

        };  // struct argument_input_2nd

        template <typename CliResT, typename = void>
        struct option_output : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_output<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::output), output_file>>>
            : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(results.output.terminal() == terminals::stdio);
                description.add_options()(
                    "output,o", po::value<std::string>()->value_name("FILE"),
                    "write output data to FILE (default: write to standard output)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("output")) {
                    results.output.assign_from_spec(varmap["output"].as<std::string>());
                }
            }

        };  // struct option_output

        template <typename CliResT, typename = void>
        struct option_output_layout : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_output_layout
        <
            CliResT,
            std::enable_if_t<std::is_same_v<decltype(CliResT::output_layout), output_file>>
        >
            : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(results.output.terminal() == terminals::stdio);
                description.add_options()(
                    "output-layout", po::value<std::string>()->value_name("FILE"),
                    "write layout data separately to FILE (default: write only one output file)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("output-layout")) {
                    results.output_layout.assign_from_spec(varmap["output-layout"].as<std::string>());
                }
            }

        };  // struct option_output_layout

        template <typename CliResT, typename = void>
        struct option_meta : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_meta<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::meta), output_file>>>
            : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(results.meta.terminal() == terminals::null);
                description.add_options()(
                    "meta,m", po::value<std::string>()->value_name("FILE"),
                    "write meta data to FILE in JSON format (default: don't write meta data)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("meta")) {
                    results.meta.assign_from_spec(varmap["meta"].as<std::string>());
                }
            }

        };  // struct option_meta

        template <typename CliResT, typename = void>
        struct option_format : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_format<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::format), fileformats>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                using namespace std::string_literals;
                auto valspec = po::value<std::string>()->value_name("SPEC");
                auto helptext = "select input file format "s;
                if (results.format == fileformats{}) {
                    helptext.append("(required)");
                    description.add_options()("format,f", std::move(valspec)->required(), helptext.c_str());
                } else {
                    helptext.append("(default: '").append(name(results.format)).append("')");
                    description.add_options()("format,f", std::move(valspec), helptext.c_str());
                }
                description.add_options()("show-formats", "show a list of the available formats and exit");
            }

            static void handle_before(CliResT& /*results*/, po::variables_map& varmap)
            {
                if (varmap.count("show-formats")) {
                    for (const auto fmt : all_fileformats()) {
                        std::cout << name(fmt) << '\n';
                    }
                    throw system_exit{};
                }
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("format")) {
                    const auto name = varmap["format"].as<std::string>();
                    results.format = value_of_fileformats(name);
                }
            }

        };  // struct option_format

        template <typename CliResT, typename = void>
        struct option_layout_2 : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_layout_2<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::layout), bool>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.layout == false);
                description.add_options()(
                    "layout,l", po::bool_switch(&results.layout),
                    "use associated layout of input graph (default: use graph data only)"
                );
            }

        };  // struct option_layout_2

        template <typename CliResT, typename = void>
        struct option_layout_3 : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_layout_3
        <
            CliResT,
            std::enable_if_t<std::is_same_v<decltype(CliResT::layout), std::optional<bool>>>
        >
            : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(!results.layout.has_value());
                description.add_options()(
                    "layout,l", po::value<bool>()->value_name("BOOL"),
                    "whether to expect and use an associated layout (default: use if and only if available)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("layout")) {
                    results.layout = varmap["layout"].as<bool>();
                }
            }

        };  // struct option_layout_3

        template <typename CliResT, typename = void>
        struct option_simplify : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_simplify<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::simplify), bool>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.simplify == false);
                description.add_options()(
                    "simplify,y", po::bool_switch(&results.simplify),
                    "delete loops and multiple edges (default: error if graph is not simple to begin with)"
                );
            }

        };  // struct option_simplify

        template <typename CliResT, typename = void>
        struct option_nodes : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_nodes<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::nodes), int>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.nodes > 0);
                const auto helptext = "desired number of nodes (default: " + std::to_string(results.nodes) + ")";
                description.add_options()(
                    "nodes,n", po::value<int>(&results.nodes)->value_name("N"),
                    helptext.c_str()
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                if (results.nodes <= 0) {
                    throw po::error{"The desired number of nodes must be positive"};
                }
            }

        };  // struct option_nodes

        template <typename CliResT, typename = void>
        struct option_torus : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_torus<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::torus), int>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.torus >= 0);
                const auto helptext = "create an N torus (default: " + std::to_string(results.torus) + ")";
                description.add_options()(
                    "torus,t", po::value<int>(&results.torus)->value_name("N"),
                    helptext.c_str()
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                if (results.torus < 0) {
                    throw po::error{"A " + std::to_string(results.torus) + " torus is not a thing"};
                }
            }

        };  // struct option_torus

        template <typename CliResT, typename = void>
        struct option_hyperdim : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_hyperdim<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::hyperdim), int>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.hyperdim > 0);
                const auto helptext = concat(
                    "project from a N-dimensional hyper space ",
                    "(default: N = ", std::to_string(results.hyperdim), ")"
                );
                description.add_options()(
                    "hyperdim,h", po::value<int>(&results.hyperdim)->value_name("N"),
                    helptext.c_str()
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                if (results.hyperdim <= 0) {
                    throw po::error{"A " + std::to_string(results.hyperdim) + "-D hyper space is not a thing"};
                }
            }

        };  // struct option_hyperdim

        template <typename CliResT, typename = void>
        struct option_symmetric : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_symmetric<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::symmetric), bool>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.symmetric == false);
                description.add_options()(
                    "symmetric,s", po::bool_switch(&results.symmetric),
                    "gemerate a perfectly symmetric graph and layout (default: generate irregular pattern)"
                );
            }

        };  // struct option_symmetric

        template <typename CliResT, typename = void>
        struct option_algorithm : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_algorithm<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::algorithm), algorithms>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                using namespace std::string_literals;
                auto valspec = po::value<std::string>()->value_name("SPEC");
                auto helptext = "select layouting algorithm "s;
                if (results.algorithm == algorithms{}) {
                    helptext.append("(required)");
                    description.add_options()("algorithm,a", std::move(valspec)->required(), helptext.c_str());
                } else {
                    helptext.append("(default: '").append(name(results.algorithm)).append("')");
                    description.add_options()("algorithm,a", std::move(valspec), helptext.c_str());
                }
                description.add_options()("show-algorithms", "show a list of the available algorithms and exit");
            }

            static void handle_before(CliResT& /*results*/, po::variables_map& varmap)
            {
                if (varmap.count("show-algorithms")) {
                    for (const auto algo : all_algorithms()) {
                        std::cout << name(algo) << '\n';
                    }
                    throw system_exit{};
                }
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("algorithm")) {
                    const auto name = varmap["algorithm"].as<std::string>();
                    results.algorithm = value_of_algorithms(name);
                }
            }

        };  // struct option_algorithm

        template <typename CliResT, typename = void>
        struct option_distribution : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_distribution
        <
            CliResT,
            std::enable_if_t<std::is_same_v<decltype(CliResT::distribution), distributions>>
        > : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                using namespace std::string_literals;
                auto valspec = po::value<std::string>()->value_name("SPEC");
                auto helptext = "select random distribution "s;
                if (results.distribution == distributions{}) {
                    helptext.append("(required)");
                    description.add_options()("distribution,d", std::move(valspec)->required(), helptext.c_str());
                } else {
                    helptext.append("(default: '").append(name(results.distribution)).append("')");
                    description.add_options()("distribution,d", std::move(valspec), helptext.c_str());
                }
                description.add_options()("show-distributions", "show a list of the available distributions and exit");
            }

            static void handle_before(CliResT& /*results*/, po::variables_map& varmap)
            {
                if (varmap.count("show-distributions")) {
                    for (const auto dist : all_distributions()) {
                        std::cout << name(dist) << '\n';
                    }
                    throw system_exit{};
                }
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("distribution")) {
                    const auto name = varmap["distribution"].as<std::string>();
                    results.distribution = value_of_distributions(name);
                }
            }

        };  // struct option_distribution

        template <typename CliResT, typename = void>
        struct option_projection : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_projection
        <
            CliResT,
            std::enable_if_t<std::is_same_v<decltype(CliResT::projection), projections>>
        > : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                using namespace std::string_literals;
                auto valspec = po::value<std::string>()->value_name("SPEC");
                auto helptext = "select geometric projection "s;
                if (results.projection == projections{}) {
                    helptext.append("(required)");
                    description.add_options()("projection,j", std::move(valspec)->required(), helptext.c_str());
                } else {
                    helptext.append("(default: '").append(name(results.projection)).append("')");
                    description.add_options()("projection,j", std::move(valspec), helptext.c_str());
                }
                description.add_options()("show-projections", "show a list of the available projections and exit");
            }

            static void handle_before(CliResT& /*results*/, po::variables_map& varmap)
            {
                if (varmap.count("show-projections")) {
                    for (const auto proj : all_projections()) {
                        std::cout << name(proj) << '\n';
                    }
                    throw system_exit{};
                }
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("projection")) {
                    const auto name = varmap["projection"].as<std::string>();
                    results.projection = value_of_projections(name);
                }
            }

        };  // struct option_projection

        template <typename CliResT, typename = void>
        struct option_rate : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_rate<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::rate), std::vector<double>>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.rate.empty());
                description.add_options()(
                    "rate,r",
                    po::value<std::vector<double>>(&results.rate)->value_name("X"),
                    "evaluate at point X in the unit interval (may be repeated to generate multiple layouts)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                for (const auto r : results.rate) {
                    if (!std::isfinite(r) || (r < 0.0) || (r > 1.0)) {
                        throw po::error{"A rate must be a real number in the closed unit interval"};
                    }
                }
            }

        };  // struct option_rate

        template <typename CliResT, typename = void>
        struct option_kernel : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_kernel<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::kernel), kernels>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                using namespace std::string_literals;
                auto valspec = po::value<std::string>()->value_name("SPEC");
                auto helptext = "aggregate and present data using the specified kernel "s;
                if (results.kernel == kernels{}) {
                    helptext.append("(required)");
                    description.add_options()("kernel,k", std::move(valspec)->required(), helptext.c_str());
                } else {
                    helptext.append("(default: '").append(name(results.kernel)).append("')");
                    description.add_options()("kernel,k", std::move(valspec), helptext.c_str());
                }
                description.add_options()("show-kernels", "show a list of the available kernels and exit");
            }

            static void handle_before(CliResT& /*results*/, po::variables_map& varmap)
            {
                if (varmap.count("show-kernels")) {
                    for (const auto kern : all_kernels()) {
                        std::cout << name(kern) << '\n';
                    }
                    throw system_exit{};
                }
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("kernel")) {
                    const auto name = varmap["kernel"].as<std::string>();
                    results.kernel = value_of_kernels(name);
                }
            }

        };  // struct option_kernel

        template <typename CliResT, typename = void>
        struct option_width : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_width<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::width), std::vector<double>>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.width.empty());
                description.add_options()(
                    "width,w",
                    po::value<std::vector<double>>(&results.width)->value_name("X"),
                    "use bin / filter width X to aggregate the data (default: use heuristic to determine optimal width)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                for (const auto w : results.width) {
                    if (!std::isfinite(w) || (w <= 0.0)) {
                        throw po::error{"The bin / filter width must be a positive real number"};
                    }
                }
            }

        };  // struct option_width

        template <typename CliResT, typename = void>
        struct option_bins : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_bins<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::bins), std::vector<int>>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.bins.empty());
                description.add_options()(
                    "bins,b",
                    po::value<std::vector<int>>(&results.bins)->value_name("N"),
                    "use N bins to aggregate the data (default: use heuristic to determine optimal count)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                for (const auto b : results.bins) {
                    if (b <= 0) {
                        throw po::error{"The number of histogram bins must be a positive integer"};
                    }
                }
            }

        };  // struct option_bins

        template <typename CliResT, typename = void>
        struct option_points : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_points<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::points), std::optional<int>>>>
            : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(!results.points.has_value());
                description.add_options()(
                    "points,p",
                    po::value<int>()->value_name("N"),
                    "evaluate smooth averages (if applicable) at N equidistant points (default: use adaptive strategy)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                if (varmap.count("points")) {
                    const auto value = varmap["points"].as<int>();
                    if (value > 0) {
                        results.points = value;
                    } else {
                        throw po::error{"The number of evaluation points must be a positive integer"};
                    }
                }
            }

        };  // struct option_points

        template <typename CliResT, typename = void>
        struct option_component : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_component<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::component), int>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.component == 0);
                const auto help1 = "analyze the major component";
                const auto help2 = "analyze the minor component";
                description.add_options()
                    ("major,1", po::value<int>(&results.component)->implicit_value(1)->zero_tokens(), help1)
                    ("minor,2", po::value<int>(&results.component)->implicit_value(2)->zero_tokens(), help2);
            }

        };  // struct option_component

        template <typename CliResT, typename = void>
        struct option_vicinity : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_vicinity
        <
            CliResT,
            std::enable_if_t<std::is_same_v<decltype(CliResT::vicinity), std::vector<double>>>
        > : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.vicinity.empty());
                description.add_options()(
                    "vicinity,v",
                    po::value<std::vector<double>>(&results.vicinity)->value_name("X"),
                    "evaluate property for vicinity X (may be repeated)"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& /*varmap*/)
            {
                for (const auto x : results.vicinity) {
                    if (!std::isfinite(x) || (x < 0.0)) {
                        throw po::error{"The vicinity must be a non-negative real number"};
                    }
                }
                if (!std::is_sorted(std::begin(results.vicinity), std::end(results.vicinity))) {
                    throw po::error{"Multiple vicinities must be specified in increasing order"};
                }
            }

        };  // struct option_vicinity

        template <typename CliResT, typename = void>
        struct option_major : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_major<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::major), point2d>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(!results.major);
                description.add_options()
                    ("major,1", po::value<point2d>(&results.major)->value_name("XY"), "draw major axis onto layout");
            }

        };  // struct option_major

        template <typename CliResT, typename = void>
        struct option_minor : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_minor<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::minor), point2d>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(!results.minor);
                description.add_options()
                    ("minor,2", po::value<point2d>(&results.minor)->value_name("XY"), "draw minor axis onto layout");
            }

        };  // struct option_minor

        template <typename CliResT, typename = void>
        struct option_clever : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_clever<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::clever), bool>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(results.clever == false);
                description.add_options()(
                    "clever,c", po::bool_switch(&results.clever),
                    "try to be smart about the initial orientation of parent layouts"
                );
            }

        };  // struct option_clever

        template <typename CliResT, typename = void>
        struct option_stress_modus : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_stress_modus
        <
            CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::stress_modus), stress_modi>>
        > : basic_option_handler<CliResT>
        {

            static void add([[maybe_unused]] CliResT& results, po::options_description& description)
            {
                assert(results.stress_modus == stress_modi::fixed);
                description.add_options()(
                    "fit-nodesep", po::bool_switch(),
                    "compute minimal stress by varying the desired node distance"
                );
                description.add_options()(
                    "fit-scale", po::bool_switch(),
                    "compute minimal stress by varying the scale of the layout"
                );
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                const auto nodesep = varmap["fit-nodesep"].as<bool>();
                const auto scale = varmap["fit-scale"].as<bool>();
                if (nodesep && scale) {
                    throw po::error{"The --fit-nodesep and --fit-scale options are mutually exclusive"};
                }
                if (nodesep) {
                    results.stress_modus = stress_modi::fit_nodesep;
                }
                if (scale) {
                    results.stress_modus = stress_modi::fit_scale;
                }
            }

        };  // struct option_stress_modus

        inline void add_color_option(const char *const name,
                                     po::options_description& description,
                                     ogdf::Color& color,
                                     const char *const objects = "objects")
        {
            const auto helptext = concat(
                "draw ", objects, " using the specified color (default: ", color.toString(), ")"
            );
            description.add_options()
                (name, po::value<std::string>()->value_name("RGB"), helptext.c_str());
        }

        inline void handle_color_option(const char *const name, ogdf::Color& color, po::variables_map& varmap)
        {
            if (varmap.count(name)) {
                const auto text = varmap[name].as<std::string>();
                if (!color.fromString(text)) {
                    throw po::error{concat("I cannot understand this color specification: --", name, "=", text)};
                }
            }
        }

        template <typename CliResT, typename = void>
        struct option_node_color : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_node_color<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::node_color), ogdf::Color>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                add_color_option("node-color", description, results.node_color, "nodes");
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                handle_color_option("node-color", results.node_color, varmap);
            }

        };  // struct option_node_color

        template <typename CliResT, typename = void>
        struct option_edge_color : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_edge_color<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::edge_color), ogdf::Color>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                add_color_option("edge-color", description, results.edge_color, "edges");
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                handle_color_option("edge-color", results.edge_color, varmap);
            }

        };  // struct option_edge_color

        template <typename CliResT, typename = void>
        struct option_axis_color : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_axis_color<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::axis_color), ogdf::Color>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                add_color_option("axis-color", description, results.axis_color, "axes");
            }

            static void handle_after(CliResT& results, po::variables_map& varmap)
            {
                handle_color_option("axis-color", results.axis_color, varmap);
            }

        };  // struct option_axis_color

        template <typename CliResT, typename = void>
        struct option_tikz : basic_option_handler<CliResT> { };

        template <typename CliResT>
        struct option_tikz<CliResT, std::enable_if_t<std::is_same_v<decltype(CliResT::tikz), bool>>>
            : basic_option_handler<CliResT>
        {

            static void add(CliResT& results, po::options_description& description)
            {
                assert(!results.tikz);
                description.add_options()(
                    "tikz", po::bool_switch(&results.tikz),
                    "output tikz code instead of SVG data");
            }

        };  // struct option_tikz

        using all_arguments_handler = argument_handler<
            argument_input, argument_input_1st, argument_input_2nd
        >;

        using all_options_handler = option_handler<
            option_output,
            option_output_layout,
            option_meta,
            option_format,
            option_layout_2,
            option_layout_3,
            option_simplify,
            option_nodes,
            option_torus,
            option_hyperdim,
            option_symmetric,
            option_algorithm,
            option_distribution,
            option_projection,
            option_rate,
            option_kernel,
            option_width,
            option_bins,
            option_points,
            option_component,
            option_vicinity,
            option_major,
            option_minor,
            option_clever,
            option_stress_modus,
            option_node_color,
            option_edge_color,
            option_axis_color,
            option_tikz
        >;

        void add_version_and_help(po::options_description& options);

        void handle_version_and_help(po::variables_map& varmap,
                                     const po::options_description& arguments,
                                     const po::options_description& options,
                                     const cli_base&);

        template <typename CliResT>
        void cli_impl_parse_args(const cli_base& cli, CliResT& results, int argc, const char *const *const argv)
        {
            auto positional = po::positional_options_description{};
            auto arguments = po::options_description{"Arguments"};
            auto options = po::options_description{"Options"};
            all_arguments_handler::add(results, arguments, positional);
            all_options_handler::add(results, options);
            add_version_and_help(options);
            auto description = po::options_description{};
            description.add(options).add(arguments);
            auto varmap = po::variables_map{};
            po::store(po::command_line_parser(argc, argv).options(description).positional(positional).run(), varmap);
            handle_version_and_help(varmap, arguments, options, cli);
            all_options_handler::handle_before(results, varmap);
            all_arguments_handler::handle_before(results, varmap);
            po::notify(varmap);
            all_options_handler::handle_after(results, varmap);
            all_arguments_handler::handle_after(results, varmap);
        }

        void init_cli_base(cli_base& cli);
        void before_main();
        void after_main();

    }  // namespace detail::cli

    template <typename AppT>
    command_line_interface<AppT>::command_line_interface(std::string prog, AppT app) :
        cli_base{std::move(prog)}, _app{std::move(app)}
    {
        detail::cli::init_cli_base(*this);
    }

    template <typename AppT>
    [[nodiscard]] int
    command_line_interface<AppT>::operator()(const int argc, const char *const *const argv)
    {
        try {
            detail::cli::cli_impl_parse_args(*this, _app.parameters, argc, argv);
            detail::cli::before_main();
            _app();
            detail::cli::after_main();
            return EXIT_SUCCESS;
        } catch (const detail::cli::system_exit& e) {
            auto message = std::string{};
            if (!check_stdio(std::cin, std::cout, message)) {
                std::cerr << prog << ": error: " << message << std::endl;
                return EXIT_FAILURE;
            }
            return e.status;
        } catch (const std::exception& e) {
            std::cerr << prog << ": error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

}  // namespace msc
