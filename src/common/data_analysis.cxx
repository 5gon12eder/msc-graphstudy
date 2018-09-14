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

#include "data_analysis.hxx"

#include <cmath>

#include "json.hxx"
#include "regression.hxx"
#include "stochastic.hxx"

namespace msc
{

    void data_analyzer::_update_info(json_object& info, json_object& subinfo, const histogram& histo) const
    {
        _update_info_common(info, subinfo);
        info["size"] = json_size{histo.size()};
        info["minimum"] = json_real{histo.min()};
        info["maximum"] = json_real{histo.max()};
        info["mean"] = json_real{histo.mean()};
        info["rms"] = json_real{histo.rms()};
        subinfo["binning"] = name(histo.binning());
        subinfo["bincount"] = json_size{histo.bincount()};
        subinfo["binwidth"] = json_real{histo.binwidth()};
        subinfo["entropy"] = json_real{histo.entropy()};
    }

    void data_analyzer::_update_info(json_object& info,
                                     json_object& subinfo,
                                     const stochastic_summary& summary,
                                     const std::optional<double> sigma,
                                     const std::optional<std::size_t> points,
                                     const std::optional<double> entropy,
                                     const std::optional<std::pair<double, double>> maxelm) const
    {
        _update_info_common(info, subinfo);
        info["size"] = msc::json_size{summary.count};
        info["minimum"] = msc::json_real{summary.min};
        info["maximum"] = msc::json_real{summary.max};
        info["mean"] = msc::json_real{summary.mean};
        info["rms"] = msc::json_real{summary.rms};
        if (sigma) {
            subinfo["sigma"] = msc::json_real{*sigma};
        }
        if (points) {
            subinfo["points"] = msc::json_size{*points};
        }
        if (entropy && std::isfinite(*entropy)) {
            subinfo["entropy"] = msc::json_real{*entropy};
        }
        if (maxelm) {
            subinfo["max-x"] = msc::json_real{maxelm->first};
            subinfo["max-y"] = msc::json_real{maxelm->second};
        }
    }

    void data_analyzer::_update_info_common(json_object& /*info*/, json_object& subinfo) const
    {
        subinfo["filename"] = make_json(_output.filename());
    }

    std::vector<std::pair<double, double>> initialize_entropies()
    {
        return {};
    }

    void append_entropy(std::vector<std::pair<double, double>>& entropies,
                        const json_object& info,
                        const std::string_view keyname,
                        const std::string_view valname)
    {
        const auto keypos = info.find(keyname);
        const auto valpos = info.find(valname);
        if ((keypos != info.cend()) && (valpos != info.cend())) {
            entropies.emplace_back(
                std::log2(std::get<msc::json_size>(keypos->second).value),
                std::get<msc::json_real>(valpos->second).value
            );
        }
    }

    void assign_entropy_regression(const std::vector<std::pair<double, double>>& entropies, json_object& info)
    {
        if (!entropies.empty()) {
            const auto [d, k] = msc::linear_regression(entropies);
            info["entropy-intercept"] = msc::json_real{d};
            info["entropy-slope"] = msc::json_real{k};
        }
    }

}  // namespace msc
