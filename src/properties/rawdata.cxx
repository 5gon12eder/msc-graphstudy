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

#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "cli.hxx"
#include "data_analysis.hxx"
#include "iosupp.hxx"
#include "json.hxx"
#include "meta.hxx"

#define PROGRAM_NAME "rawdata"

namespace /*anonymous*/
{

    std::vector<double> load_events(const msc::input_file& src)
    {
        auto events = std::vector<double>{};
        auto stream = boost::iostreams::filtering_istream{};
        const auto name = msc::prepare_stream(stream, src);
        auto line = std::string{};
        while (std::getline(stream, line)) {
            auto str = line.c_str();
            while (true) {
                const int c = static_cast<unsigned char>(*str);
                if ((c == '\0') || (c == '#'))  goto next_line;
                if (!std::isspace(c))           break;
                ++str;
            }
            {   // We use a nested scope so we can goto after it
                char* endptr = nullptr;
                errno = 0;
                const auto value = std::strtod(str, &endptr);
                if (errno != 0) {
                    msc::report_io_error(name, std::strerror(errno));
                }
                while (*endptr != '\0') {
                    const int c = static_cast<unsigned char>(*endptr);
                    if (c == '#')          break;
                    if (!std::isspace(c))  msc::report_io_error(name, "Not a single floating-point value: " + line);
                    ++endptr;
                }
                events.push_back(value);
            }
        next_line:
            continue;
        }
        if (!stream.eof()) {
            msc::report_io_error(name, "Cannot read raw data");
        }
        return events;
    }

    struct application final
    {
        msc::cli_parameters_property parameters{};
        void operator()() const;
    };

    msc::json_object basic_info()
    {
        auto info = msc::json_object{};
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    void application::operator()() const
    {
        const auto events = load_events(this->parameters.input);
        auto info = basic_info();
        auto subinfos = msc::json_array{};
        auto analyzer = msc::data_analyzer{this->parameters.kernel};
        auto entropies = msc::initialize_entropies();
        for (std::size_t i = 0; i < this->parameters.iterations(); ++i) {
            auto subinfo = msc::json_object{};
            analyzer.set_width(msc::get_item(this->parameters.width, i));
            analyzer.set_bins(msc::get_item(this->parameters.bins, i));
            analyzer.set_points(this->parameters.points);
            analyzer.set_output(msc::expand_filename(this->parameters.output, i));
            analyzer.analyze(std::begin(events), std::end(events), info, subinfo);
            msc::append_entropy(entropies, subinfo, "bincount");
            subinfos.push_back(std::move(subinfo));
        }
        info["data"] = std::move(subinfos);
        msc::assign_entropy_regression(entropies, info);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Generic data analysis tool for existing raw data.");
    app.help.push_back(msc::helptext_file_name_expansion());
    return app(argc, argv);
}
