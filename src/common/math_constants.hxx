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
 * @file math_constants.hxx
 *
 * @brief
 *     Portability header that ensures the `M_*` constants from the `<cmath>` header are available.
 *
 * @warning
 *     This header file includes the `<cmath>` header and then conditionally defines those macros that are still not
 *     defined.  Since the `M_*` macro name-space is reserved, this will -- strictly speaking -- trigger undefined
 *     behavior.  This header will *not* define any magic macros beforehand (such as `_XOPEN_SOURCE` for GCC or
 *     `_USE_MATH_DEFINES` for MSVC).  If such tricks are needed for your compiler, please make sure that the respective
 *     definitions are passed on the compiler command-line.
 *
 */

#ifndef MSC_MATH_CONSTANTS_HXX
#define MSC_MATH_CONSTANTS_HXX

#include <cmath>

/**
 * @def M_PI
 *
 * @brief
 *     Constant expression with the value of mathematical constant &pi; (&pi; &asymp; 3.141592653589793).
 *
 */
#ifndef M_PI
#  define M_PI 0x1.921fb54442d18p+1
#endif

/**
 * @def M_E
 *
 * @brief
 *     Constant expression with the value of Euler's number (<var>e</var> &asymp; 2.718281828459045).
 *
 */
#ifndef M_E
#  define M_E 0x1.5bf0a8b145769p+1
#endif

/**
 * @def M_SQRT2
 *
 * @brief
 *     Constant expression with the value of the square root of two (2<sup>1/2</sup> &asymp; 1.414213562373095).
 *
 */
#ifndef M_SQRT2
#  define M_SQRT2 0x1.6a09e667f3bcdp+0
#endif

#endif  // !defined(MSC_MATH_CONSTANTS_HXX)
