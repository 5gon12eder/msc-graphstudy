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
 * @file ogdf_fwd.hxx
 *
 * @brief
 *     Forward declarations for frequently used interface-types from the OGDF.
 *
 */

#ifndef MSC_OGDF_FWD_HXX
#define MSC_OGDF_FWD_HXX

/**
 * @brief
 *     Forward declarations for frequently used interface-types from the OGDF.
 *
 */
namespace ogdf
{

    class Graph;
    class GraphAttributes;

    class NodeElement;
    class EdgeElement;

    typedef NodeElement* node;
    typedef EdgeElement* edge;

}  // namespace ogdf

#endif  // !defined(MSC_OGDF_FWD_HXX)
