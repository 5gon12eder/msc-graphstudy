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

#include "stress.hxx"

#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <utility>

#include "normalizer.hxx"
#include "useful.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        constexpr auto enable_verbose_debugging = false;

        using array3d = std::array<double, 3>;

        [[maybe_unused]] void
        print_array3d(std::ostream& ostr, const array3d& values, const std::string_view name, const int width)
        {
            ostr << name << " = [ ";
            for (auto it = values.begin(); it != values.end(); ++it) {
                ostr << ((it == values.begin()) ? "" : ", ") << std::setw(width) << *it;
            }
            ostr << " ]\n";
        }

        template <typename FuncT>
        [[maybe_unused]] bool is_decent_result(
            FuncT&& f,
            const array3d& x,
            const array3d& y,
            const parabola_result& res,
            const std::string_view name) noexcept
        {
            if (enable_verbose_debugging) {
                std::clog << std::setprecision(5) << std::scientific;
                print_array3d(std::clog, x, "x", 10);
                print_array3d(std::clog, y, "y", 10);
                std::clog << res << "\n";
            }
            constexpr auto tol = 1.1920929E-07;  // epsilon(float32)
            const auto perform_check = [](const char *const message, const bool result) -> int {
                if (enable_verbose_debugging) {
                    std::clog << message << " ...  " << (result ? "yes" : "no") << "\n";
                }
                return result ? 0 : 1;
            };
            auto issues = 0;
            issues += perform_check("Checking whether the leading term has a positive coefficient", (res.c > -tol));
            issues += perform_check("Checking whether x0 is positive", (res.x0 > -tol));
            issues += perform_check("Checking whether y0 is non-negative", (res.y0 >= -tol));
            const auto y0_div = std::max(1.0, std::abs(res.y0));
            issues += perform_check("Checking whether f(x0) = y0", (std::abs(f(res.x0) - res.y0) / y0_div < tol));
            const auto y_min = *std::min_element(std::begin(y), std::end(y));
            issues += perform_check(
                "Checking whether y0 <= min { y }",
                ((std::abs(res.y0 - y_min) / y0_div < tol) || (res.y0 <= y_min))
            );
            if (enable_verbose_debugging) {
                std::clog << name << ": ";
                if (issues == 0) { std::clog << "OK"; } else { std::clog << issues << " problems"; }
                std::clog << "\n" << std::flush;
            }
            return (issues == 0);
        }

        parabola_result get_default_answer() noexcept
        {
            auto result = parabola_result{};
            result.x0 = default_node_distance;
            return result;
        }

        constexpr parabola_result fit_parabola(const array3d& x, const array3d& y)
        {
            auto result = parabola_result{};
            const double matrix[3][3] = {
                {1.0, x[0], square(x[0])},
                {1.0, x[1], square(x[1])},
                {1.0, x[2], square(x[2])},
            };
            const auto determinant = 0.0
                + matrix[0][0] * matrix[1][1] * matrix[2][2]
                + matrix[0][1] * matrix[1][2] * matrix[2][0]
                + matrix[0][2] * matrix[1][0] * matrix[2][1]
                - matrix[0][2] * matrix[1][1] * matrix[2][0]
                - matrix[0][1] * matrix[1][0] * matrix[2][2]
                - matrix[0][0] * matrix[1][2] * matrix[2][1];
            const double inverse[3][3] = {
                {
                    (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) / determinant,
                    (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) / determinant,
                    (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / determinant,
                },
                {
                    (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) / determinant,
                    (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) / determinant,
                    (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / determinant,
                },
                {
                    (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) / determinant,
                    (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) / determinant,
                    (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) / determinant,
                },
            };
            result.a = inverse[0][0] * y[0] + inverse[0][1] * y[1] + inverse[0][2] * y[2];
            result.b = inverse[1][0] * y[0] + inverse[1][1] * y[1] + inverse[1][2] * y[2];
            result.c = inverse[2][0] * y[0] + inverse[2][1] * y[1] + inverse[2][2] * y[2];
            result.x0 = -0.5 * result.b / result.c;
            result.y0 = result.a + result.b * result.x0 + result.c * square(result.x0);
            return result;
        }

    }  // namespace /*anonymous*/

    double compute_stress(const ogdf::GraphAttributes& attrs, const double nodesep)
    {
        const auto matrix = get_pairwise_shortest_paths(attrs.constGraph());
        const auto infty = attrs.constGraph().numberOfNodes() + 1.0;
        const auto terms = pairwise_stress{attrs, *matrix, nodesep, infty};
        return std::accumulate(std::begin(terms), std::end(terms), 0.0);
    }

    parabola_result compute_stress_fit_nodesep(const ogdf::GraphAttributes& attrs)
    {
        if (attrs.constGraph().numberOfEdges() < 1) {
            return get_default_answer();
        }
        const auto matrix = get_pairwise_shortest_paths(attrs.constGraph());
        const auto infty = attrs.constGraph().numberOfNodes() + 1.0;
        const auto computer = [ap = &attrs, mp = matrix.get(), infty](const double nodesep){
            const auto terms = pairwise_stress{*ap, *mp, nodesep, infty};
            return std::accumulate(std::begin(terms), std::end(terms), 0.0);
        };
        const auto nodesep = array3d{{
            0.1 * default_node_distance,
            0.5 * default_node_distance,
            1.0 * default_node_distance,
        }};
        const auto stress = array3d{{computer(nodesep[0]), computer(nodesep[1]), computer(nodesep[2])}};
        const auto result = fit_parabola(nodesep, stress);
        assert(is_decent_result(computer, nodesep, stress, result, "STRESS_FIT_NODESEP"));
        return result;
    }

    parabola_result compute_stress_fit_scale(const ogdf::GraphAttributes& attrs)
    {
        if (attrs.constGraph().numberOfEdges() < 1) {
            return get_default_answer();
        }
        const auto matrix = get_pairwise_shortest_paths(attrs.constGraph());
        const auto infty = attrs.constGraph().numberOfNodes() + 1.0;
        const auto computer = [ap = &attrs, mp = matrix.get(), infty](const double scale){
            const auto acopy = std::make_unique<ogdf::GraphAttributes>(*ap);
            acopy->scale(scale, false);
            const auto terms = pairwise_stress{*acopy, *mp, default_node_distance, infty};
            return std::accumulate(std::begin(terms), std::end(terms), 0.0);
        };
        const auto scale = array3d{{0.5, 1.0, 1.5}};
        const auto stress = array3d{{computer(scale[0]), computer(scale[1]), computer(scale[2])}};
        const auto result = fit_parabola(scale, stress);
        assert(is_decent_result(computer, scale, stress, result, "STRESS_FIT_SCALE"));
        return result;
    }

    std::ostream& operator<<(std::ostream& ostr, const parabola_result& pr)
    {
        const auto sign = [](const auto x){ return (x < 0) ? '-' : '+'; };
        return ostr << "f(x) = " << pr.c << " * x**2 "
                    << sign(pr.b) << " " << std::abs(pr.b) << " * x "
                    << sign(pr.a) << " " << std::abs(pr.a)
                    << " with f(" << pr.x0 << ") = " << pr.y0;
    }

}  // namespace msc
