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

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

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
#include "point.hxx"
#include "random.hxx"
#include "useful.hxx"

#define PROGRAM_NAME "movlsq"

namespace /*anonymous*/
{

    class matrix2x2 final
    {
    public:

        constexpr matrix2x2() noexcept = default;

        constexpr double& operator()(int i, int j)
        {
            assert((i >= 0) && (i < 2));
            assert((j >= 0) && (j < 2));
            return _data[i][j];
        }

        constexpr const double& operator()(int i, int j) const
        {
            assert((i >= 0) && (i < 2));
            assert((j >= 0) && (j < 2));
            return _data[i][j];
        }

        matrix2x2& operator+=(const matrix2x2& other) noexcept
        {
            _data[0][0] += other._data[0][0];
            _data[0][1] += other._data[0][1];
            _data[1][0] += other._data[1][0];
            _data[1][1] += other._data[1][1];
            return *this;
        }

        friend constexpr matrix2x2 operator*(const double alpha, const matrix2x2& matrix) noexcept
        {
            auto m = matrix2x2{};
            m._data[0][0] = alpha * matrix._data[0][0];
            m._data[0][1] = alpha * matrix._data[0][1];
            m._data[1][0] = alpha * matrix._data[1][0];
            m._data[1][1] = alpha * matrix._data[1][1];
            return m;
        }

    private:

        double _data[2][2] = {0};

    };  // class matrix2x2

    constexpr matrix2x2 outer(const msc::point2d& u, const msc::point2d& v) noexcept
    {
        auto m = matrix2x2{};
        m(0, 0) += u[0] * v[0];
        m(0, 1) += u[0] * v[1];
        m(1, 0) += u[1] * v[0];
        m(1, 1) += u[1] * v[1];
        return m;
    }

    constexpr double det(const matrix2x2& m) noexcept
    {
        return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
    }

    constexpr matrix2x2 invert(const matrix2x2& m) noexcept
    {
        const auto d = det(m);
        auto inv = matrix2x2{};
        inv(0, 0) = +m(1, 1) / d;
        inv(0, 1) = -m(0, 1) / d;
        inv(1, 0) = -m(1, 0) / d;
        inv(1, 1) = +m(0, 0) / d;
        return inv;
    }

    constexpr double braket(const msc::point2d& u, const matrix2x2& m, const msc::point2d& v) noexcept
    {
        return u[0] * (m(0, 0) * v[0] + m(0, 1) * v[1]) + u[1] * (m(1, 0) * v[0] + m(1, 1) * v[1]);
    }

    class movlsq_worsener final
    {
    public:

        template <typename EngineT>
        movlsq_worsener(EngineT& engine, const ogdf::Graph& graph)
        {
            auto ctrldist = std::geometric_distribution{1.0 / std::max(1.0, std::sqrt(graph.numberOfNodes()))};
            auto unitdist = std::uniform_real_distribution<double>{0.0, 1.0};
            const auto pointgen = [&engine, &unitdist](){ return msc::make_random_point<double, 2>(engine, unitdist); };
            const auto ctrls = std::max(5, ctrldist(engine));
            _controls_src.reserve(ctrls);
            _controls_dst.reserve(ctrls);
            std::generate_n(std::back_inserter(_controls_src), ctrls, pointgen);
            std::generate_n(std::back_inserter(_controls_dst), ctrls, pointgen);
        }

        std::size_t controls() const noexcept
        {
            return msc::get_same({_controls_src.size(), _controls_dst.size()});
        }

        const std::vector<msc::point2d>& controls_src() const noexcept
        {
            return _controls_src;
        }

        const std::vector<msc::point2d>& controls_dst() const noexcept
        {
            return _controls_dst;
        }

        std::unique_ptr<ogdf::GraphAttributes>
        operator()(const ogdf::GraphAttributes& attrs, const double rate) const
        {
            const auto bbox = msc::get_bounding_box(attrs);
            const auto org = bbox.first;
            const auto scale = abs(bbox.second - bbox.first);
            const auto n = this->controls();
            auto dst = std::vector<msc::point2d>(n);
            for (std::size_t i = 0; i < n; ++i) {
                dst[i] = (1.0 - rate) * _controls_src[i] + rate * _controls_dst[i];
            }
            auto work2a = std::vector<msc::point2d>(n);
            auto work2b = std::vector<msc::point2d>(n);
            auto work1a = std::vector<double>(n);
            auto work1b = std::vector<double>(n);
            auto worse = std::make_unique<ogdf::GraphAttributes>(attrs.constGraph());
            for (const auto v : attrs.constGraph().nodes) {
                const auto oldpos = (msc::point2d{attrs.x(v), attrs.y(v)} - org) / scale;
                const auto newpos = _transform(_controls_src, dst, oldpos, work2a, work2b, work1a, work1b);
                // Don't bother transforming the relative coordinates back.  We're gonna normalize the layout anyhow.
                worse->x(v) = newpos.x();
                worse->y(v) = newpos.y();
            }
            msc::normalize_layout(*worse);
            return worse;
        }

    private:

        std::vector<msc::point2d> _controls_src{};
        std::vector<msc::point2d> _controls_dst{};

        // The variable names in this function follow as closely as possible the notation used in section 2 and 2.1 of
        // the paper by Schaefer et al.  The vector inputs must all be of the same length.  Those that are not
        // const-qualified are used as working buffers.  Their content at entry is ignored and their content at exit is
        // unspecified.  No size-changing functions will be performed on them.  In fact, this function does not allocate
        // memory at all.
        msc::point2d _transform(const std::vector<msc::point2d>& p,
                                const std::vector<msc::point2d>& q,
                                const msc::point2d& v,
                                std::vector<msc::point2d>& p_hat,
                                std::vector<msc::point2d>& q_hat,
                                std::vector<double>& w,
                                std::vector<double>& a) const noexcept
        {
            const auto n = msc::get_same({p.size(), q.size(), p_hat.size(), q_hat.size(), w.size(), a.size()});
            std::transform(
                std::begin(p), std::end(p), std::begin(w),
                [v, alpha = 1.0](const msc::point2d& pi){ return std::pow(normsq(pi - v), -alpha); }
            );
            const auto wsum = std::accumulate(std::begin(w), std::end(w), 0.0);
            const auto p_star = std::inner_product(std::begin(p), std::end(p), std::begin(w), msc::point2d{}) / wsum;
            const auto q_star = std::inner_product(std::begin(q), std::end(q), std::begin(w), msc::point2d{}) / wsum;
            std::transform(
                std::begin(p), std::end(p), std::begin(p_hat), [p_star](const msc::point2d& pi){ return pi - p_star; }
            );
            std::transform(
                std::begin(q), std::end(q), std::begin(q_hat), [q_star](const msc::point2d& qi){ return qi - q_star; }
            );
            auto matrix = matrix2x2{};
            for (std::size_t i = 0; i < n; ++i) {
                matrix += w[i] * outer(p_hat[i], p_hat[i]);
            }
            const auto inverse = invert(matrix);
            for (std::size_t i = 0; i < n; ++i) {
                a[i] = braket(msc::point2d{v - p_star}, inverse, w[i] * p_hat[i]);
            }
            return std::inner_product(std::begin(a), std::end(a), std::begin(q_hat), q_star);
        }

    };  // struct movlsq_worsener

    struct application final
    {
        msc::cli_parameters_worsening parameters{};
        void operator()() const;
    };

    msc::json_array coordinate_list_to_json(const std::vector<msc::point2d>& coords)
    {
        auto array = msc::json_array{};
        for (const auto& p : coords) {
            array.emplace_back(msc::json_array{msc::json_real{p.x()}, msc::json_real{p.y()}});
        }
        return array;
    }

    msc::json_object get_info(const movlsq_worsener worsener, const std::string& seed)
    {
        auto info = msc::json_object{};
        info["controls"] = msc::json_size{worsener.controls()};
        info["controls-src"] = coordinate_list_to_json(worsener.controls_src());
        info["controls-dst"] = coordinate_list_to_json(worsener.controls_dst());
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
        auto [graph, attrs] = msc::load_layout(this->parameters.input);
        const auto worsener = movlsq_worsener{rndeng, *graph};
        auto info = get_info(worsener, seed);
        auto data = msc::json_array{};
        for (const auto rate : this->parameters.rate) {
            const auto dest = this->parameters.expand_filename(rate);
            const auto worse = worsener(*attrs, rate);
            msc::store_layout(*worse, dest);
            data.push_back(get_subinfo(*worse, rate, dest.filename()));
        }
        info["data"] = std::move(data);
        msc::print_meta(info, this->parameters.meta);
    }

}  // namespace /*anonymous*/

int main(const int argc, const char *const *const argv)
{
    auto app = msc::command_line_interface<application>{PROGRAM_NAME};
    app.help.push_back(
        "Worsens a given layout by distorting node positions according to the \"Moving Least Squares\" algorithm"
        " proposed by Schaefer et al."
    );
    return app(argc, argv);
}
