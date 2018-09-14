// -*- coding:utf-8; mode:c++; -*-

// Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
//
// Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
// provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "sliding.hxx"
#include "stochastic.hxx"  // for msc::square

int main(int argc, char** argv)
{
    auto adaptive = false;
    if (argc > 2) {
        std::fprintf(stderr, "%s: error: too many arguments\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        if (std::strcmp(argv[1], "--adaptive") == 0) {
            adaptive = true;
        } else {
            std::fprintf(stderr, "%s: error: unknown argument: %s\n", argv[0], argv[1]);
            return EXIT_FAILURE;
        }
    }
    const auto lambda = [](const double x){
        constexpr auto gamma = 1.0 / std::sqrt(2.0);
        return 10.0 * std::sin(2.0 * x) / (M_PI * gamma * (1.0 + msc::square((x - 3.0) / gamma)));
    };
    const auto xmin =  0.0;
    const auto xmax = 10.0;
    if (adaptive) {
        const auto density = msc::make_density_adaptive(lambda, xmin, xmax, false);
        std::printf("# Adaptive sampling (N = %d)\n\n", static_cast<int>(density.size()));
        for (const auto [x, y] : density) {
            std::printf("%20.10E %20.10E\n", x, y);
        }
    } else {
        const auto n = 1000;
        std::printf("# Equidistant sampling (N = %d)\n\n", n);
        for (auto i = 0; i <= n; ++i) {
            const auto x = xmin + i * (xmax - xmin) / n;
            const auto y = lambda(x);
            std::printf("%20.10E %20.10E\n", x, y);
        }
    }
}
