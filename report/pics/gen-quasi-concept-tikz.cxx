// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
//
// Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
// provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

// This program could be expressed more succinctly in tikz.  However, it turns out that TeX' arithmetic precision is not
// fit for this task and the generated drawing is imprecise above the point where the graphical result is acceptable.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

#include "point.hxx"

int main(int argc, char** argv)
{
    if (argc > 1) {
        std::fprintf(stderr, "%s: error: too many arguments\n", argv[0]);
        return EXIT_FAILURE;
    }
    const auto golden = msc::normalized(msc::point2d{1.0, (1.0 + std::sqrt(5)) / 2.0});
    const auto origin = msc::point2d{1.25, -0.5};
    const auto thickness = 0.5;
    const auto n = 9;
    const auto m = 12;
    auto lattice = std::vector<msc::point2d>{};
    auto projected = std::map<msc::point2d, msc::point2d, msc::point_order<double>>{};
    for (auto i = 0; i < n; ++i) {
        for (auto j = 0; j < m; ++j) {
            lattice.emplace_back(static_cast<double>(j), static_cast<double>(i));
        }
    }
    for (const auto latt : lattice) {
        const auto hypo = latt - origin;
        const auto para = dot(golden, hypo) * golden;
        const auto perp = hypo - para;
        if (abs(perp) <= thickness) {
            const auto proj = origin + para;
            projected[latt] = proj;
        }
    }
    const auto strokelength = abs(std::max_element(
        std::begin(projected), std::end(projected),
        [origin](auto&& lhs, auto&& rhs){ return distance(origin, lhs.second) < distance(origin, rhs.second); }
    )->second - origin);
    const auto target = origin + 1.05 * strokelength * golden;
    const auto ortho = msc::point2d{-golden.y(), golden.x()};
    const auto org_1 = origin + thickness * ortho;
    const auto tgt_1 = target + thickness * ortho;
    const auto org_2 = origin - thickness * ortho;
    const auto tgt_2 = target - thickness * ortho;
    const auto overtick = 0.2;
    std::printf("%% -*- coding:utf-8; mode:latex; -*- %%\n");
    std::printf("\n");
    std::printf("\\begin{tikzpicture}\n");
    std::printf("\n");
    for (auto i = 0; i < n; ++i) {
        const auto x1 = -overtick;
        const auto x2 = static_cast<double>(m - 1) + overtick;
        const auto y = static_cast<double>(i);
        std::printf("\t\\draw[ultra thin] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", x1, y, x2, y);
    }
    for (auto j = 0; j < m; ++j) {
        const auto x = static_cast<double>(j);
        const auto y1 = -overtick;
        const auto y2 = static_cast<double>(n - 1) + overtick;
        std::printf("\t\\draw[ultra thin] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", x, y1, x, y2);
    }
    std::printf("\n");
    for (const auto [x, y] : lattice) {
        std::printf("\t\\node[vertex] at (%10.7f, %10.7f) {};\n", x, y);
    }
    std::printf("\n");
    for (const auto& [latt, proj] : projected) {
        std::printf("\t\\node[vertex] at (%10.7f, %10.7f) {};\n", proj.x(), proj.y());
        std::printf("\t\\draw[thin] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", latt.x(), latt.y(), proj.x(), proj.y());
    }
    std::printf("\n");
    std::printf("\t\\draw (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", origin.x(), origin.y(), target.x(), target.y());
    std::printf("\t\\draw[dashed] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", org_1.x(), org_1.y(), tgt_1.x(), tgt_1.y());
    std::printf("\t\\draw[dashed] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n", org_2.x(), org_2.y(), tgt_2.x(), tgt_2.y());
    std::printf("\n");
    for (auto it1 = std::begin(projected); it1 != std::end(projected); ++it1) {
        for (auto it2 = std::next(it1); it2 != std::end(projected); ++it2) {
            if (std::abs(1.0 - distance(it1->first, it2->first)) < 1.0E-10) {
                std::printf("\t\\draw[very thick] (%10.7f, %10.7f) -- (%10.7f, %10.7f);\n",
                            it1->second.x(), it1->second.y(), it2->second.x(), it2->second.y());
            }
        }
    }
    std::printf("\n");
    std::printf("\\end{tikzpicture}\n");
}
