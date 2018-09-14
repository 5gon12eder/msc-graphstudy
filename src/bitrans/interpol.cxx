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

#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "cli.hxx"
#include "file.hxx"
#include "fingerprint.hxx"
#include "io.hxx"
#include "json.hxx"
#include "meta.hxx"
#include "normalizer.hxx"
#include "ogdf_fix.hxx"
#include "ogdf_fix.hxx"
#include "point.hxx"
#include "princomp.hxx"
#include "random.hxx"
#include "stochastic.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "interpol"

namespace /*anonymous*/
{

    using ogdf_vertex_coordinates = ogdf::NodeArray<msc::point2d>;

    std::vector<msc::point2d> get_all_coordinates(const ogdf::GraphAttributes& attrs)
    {
        auto coords = std::vector<msc::point2d>{};
        coords.reserve(attrs.constGraph().numberOfNodes());
        for (const auto v : attrs.constGraph().nodes) {
            coords.push_back(msc::get_coords(attrs, v));
        }
        return coords;
    }

    template <typename EngineT>
    msc::point2d
    get_principial_layout(EngineT& engine, const ogdf::GraphAttributes& attrs, ogdf_vertex_coordinates& principial)
    {
        const auto coords = get_all_coordinates(attrs);
        const auto [major, minor] = msc::find_primary_axes_nondestructive(coords, engine);
        const auto extractor = [&coords](const auto& axis){
            const auto getter = [axis](const auto& p){ return dot(axis, p); };
            const auto first = boost::make_transform_iterator(std::begin(coords), getter);
            const auto last = boost::make_transform_iterator(std::end(coords), getter);
            return msc::mean_stdev(first, last);
        };
        const auto major_stdev = extractor(major).second;
        const auto minor_stdev = extractor(minor).second;
        principial.init(attrs.constGraph());
        for (const auto v : attrs.constGraph().nodes) {
            const auto c = msc::get_coords(attrs, v);
            const auto x = dot(major, c);
            const auto y = dot(minor, c);
            principial[v] = msc::point2d{x, y};
        }
        return {major_stdev, minor_stdev};
    }

    double principial_layout_distance(const ogdf_vertex_coordinates& lhs, const msc::point2d lhsdev,
                                      const ogdf_vertex_coordinates& rhs, const msc::point2d rhsdev)
    {
        const auto n = msc::get_same({lhs.graphOf()->numberOfNodes(), rhs.graphOf()->numberOfNodes()});
        auto lhsit = lhs.begin();
        auto rhsit = rhs.begin();
        auto accu = 0.0;
        for (auto i = 0; i < n; ++i) {
            // Overloading operator-> for an iterator would be too much to ask for...
            const auto l = msc::point2d{(*lhsit).x() / lhsdev.x(), (*lhsit).y() / lhsdev.y()};
            const auto r = msc::point2d{(*rhsit).x() / rhsdev.x(), (*rhsit).y() / rhsdev.y()};
            accu += msc::normsq(r - l);
            ++lhsit;
            ++rhsit;
        }
        return accu;
    }

    class linear_interpolator final
    {
    public:

        template <typename EngineT>
        linear_interpolator(EngineT& engine,
                            const ogdf::GraphAttributes& lhs,
                            const ogdf::GraphAttributes& rhs,
                            const bool clever)
            : _lhs_pl{std::make_unique<ogdf_vertex_coordinates>()}, _rhs_pl{std::make_unique<ogdf_vertex_coordinates>()}
        {
            if (clever) {
                const auto lhsdev = get_principial_layout(engine, lhs, *_lhs_pl);
                const auto rhsdev = get_principial_layout(engine, rhs, *_rhs_pl);
                auto bestdiff = std::numeric_limits<double>::infinity();
                auto bestsign = msc::point2d{};
                for (const auto sx : {-1.0, +1.0}) {
                    for (const auto sy : {-1.0, +1.0}) {
                        const auto signrhsdev = msc::point2d{sx * rhsdev.x(), sy * rhsdev.y()};
                        const auto diff = principial_layout_distance(*_lhs_pl, lhsdev, *_rhs_pl, signrhsdev);
                        if (diff < bestdiff) {
                            bestdiff = diff;
                            bestsign = {sx, sy};
                        }
                    }
                }
                assert(bestsign);
                for (auto it = _rhs_pl->begin(); it != _rhs_pl->end(); ++it) {
                    (*it).x() *= bestsign.x();
                    (*it).y() *= bestsign.y();
                }
            } else {
                _lhs_pl->init(lhs.constGraph());
                for (const auto v : lhs.constGraph().nodes) {
                    (*_lhs_pl)[v] = msc::get_coords(lhs, v);
                }
                _rhs_pl->init(rhs.constGraph());
                for (const auto v : rhs.constGraph().nodes) {
                    (*_rhs_pl)[v] = msc::get_coords(rhs, v);
                }
            }
        }

        std::unique_ptr<ogdf::GraphAttributes> operator()(const double rate) const
        {
            auto inter = std::make_unique<ogdf::GraphAttributes>(*_lhs_pl->graphOf());
            auto lhsit = _lhs_pl->begin();
            auto rhsit = _rhs_pl->begin();
            for (const auto v : inter->constGraph().nodes) {
                assert(lhsit != _lhs_pl->end());
                assert(rhsit != _rhs_pl->end());
                const auto middle = (1.0 - rate) * (*lhsit) + rate * (*rhsit);
                inter->x(v) = middle.x();
                inter->y(v) = middle.y();
                ++lhsit;
                ++rhsit;
            }
            assert(lhsit == _lhs_pl->end());
            assert(rhsit == _rhs_pl->end());
            msc::normalize_layout(*inter);
            return inter;
        }

    private:

        std::unique_ptr<ogdf_vertex_coordinates> _lhs_pl{};
        std::unique_ptr<ogdf_vertex_coordinates> _rhs_pl{};

    };  // class linear_interpolator

    struct application final
    {
        msc::cli_parameters_interpolation parameters{};
        void operator()() const;
    };

    msc::json_object get_info(const std::string& seed)
    {
        auto info = msc::json_object{};
        info["seed"] = seed;
        info["producer"] = PROGRAM_NAME;
        return info;
    }

    msc::json_object get_subinfo(const ogdf::GraphAttributes& attrs, const double rate, const std::string& filename)
    {
        const auto bbox = msc::get_bounding_box_size(attrs);
        auto info = msc::json_object{};
        info["filename"] = filename;
        info["layout"] = msc::layout_fingerprint(attrs);
        info["rate"] = msc::json_real{rate};
        info["width"] = msc::json_real{bbox.x()};
        info["height"] = msc::json_real{bbox.y()};
        return info;
    }

    void application::operator()() const
    {
        auto rndeng = std::mt19937{};
        const auto seed = msc::seed_random_engine(rndeng);
        auto [graph1st, attrs1st] = msc::load_layout(this->parameters.input1st);
        auto [graph2nd, attrs2nd] = msc::load_layout(this->parameters.input2nd);
        const auto interpolator = linear_interpolator{rndeng, *attrs1st, *attrs2nd, this->parameters.clever};
        auto info = get_info(seed);
        auto data = msc::json_array{};
        for (const auto rate : this->parameters.rate) {
            const auto dest = this->parameters.expand_filename(rate);
            const auto inter = interpolator(rate);
            msc::store_layout(*inter, dest);
            data.push_back(get_subinfo(*inter, rate, dest.filename()));
        }
        info["data"] = std::move(data);
        msc::print_meta(info, this->parameters.meta);
        (void) graph1st.get();
        (void) graph2nd.get();
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back("Linear interpolation between layouts.");
    return app(argc, argv);
}
