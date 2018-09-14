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
#include <config.h>
#endif

#include "normalizer.hxx"

#include <cassert>
#include <cmath>
#include <stdexcept>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

#include "point.hxx"

namespace msc
{

    namespace /*anonymous*/
    {

        const auto default_node_size = 5.0;

        // We perform this check always rather than asserting on this property because programming against the OGDF is
        // already scary enough so we rather widen our contract just a little bit at this point.
        void check_layout_finite(const ogdf::GraphAttributes& attrs)
        {
            for (const auto v : attrs.constGraph().nodes) {
                if (!std::isfinite(attrs.x(v)) ||!std::isfinite(attrs.y(v))) {
                    throw std::invalid_argument{"Cannot normalize layout if coordinates are non-finite to begin with"};
                }
            }
        }

        void normalize_layout_translate(ogdf::GraphAttributes& attrs)
        {
            assert(attrs.constGraph().numberOfNodes() > 0);
            auto xsum = 0.0;
            auto ysum = 0.0;
            for (const auto v : attrs.constGraph().nodes) {
                xsum += attrs.x(v);
                ysum += attrs.y(v);
            }
            const auto n = attrs.constGraph().numberOfNodes();
            const auto xmean = xsum / n;
            const auto ymean = ysum / n;
            attrs.translate(-xmean, -ymean);
        }

        void normalize_layout_scale_connected(ogdf::GraphAttributes& attrs)
        {
            assert(attrs.constGraph().numberOfEdges() > 0);
            auto dsum = 0.0;
            for (const auto e : attrs.constGraph().edges) {
                const auto v1 = e->source();
                const auto v2 = e->target();
                const auto p1 = msc::point2d{attrs.x(v1), attrs.y(v1)};
                const auto p2 = msc::point2d{attrs.x(v2), attrs.y(v2)};
                dsum += distance(p1, p2);
            }
            const auto dmean = dsum / attrs.constGraph().numberOfEdges();
            attrs.scale(default_node_distance / dmean, false);
        }

        void normalize_layout_scale_disconnected(ogdf::GraphAttributes& attrs)
        {
            assert(attrs.constGraph().numberOfNodes() > 1);
            auto dsum = 0.0;
            auto tally = 0;
            for (auto v1 = attrs.constGraph().firstNode(); v1 != nullptr; v1 = v1->succ()) {
                for (auto v2 = v1->succ(); v2 != nullptr; v2 = v2->succ()) {
                    const auto p1 = msc::point2d{attrs.x(v1), attrs.y(v1)};
                    const auto p2 = msc::point2d{attrs.x(v2), attrs.y(v2)};
                    dsum += distance(p1, p2);
                    tally += 1;
                }
            }
            const auto dmean = dsum / tally;
            attrs.scale(default_node_distance / dmean, false);
        }

        void normalize_node_shapes(ogdf::GraphAttributes& attrs)
        {
            for (const auto v : attrs.constGraph().nodes) {
                attrs.width(v)  = default_node_size;
                attrs.height(v) = default_node_size;
            }
        }

    }  // namespace /*anonymous*/

    void normalize_layout(ogdf::GraphAttributes& attrs)
    {
        assert(attrs.has(ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics));
        check_layout_finite(attrs);
        if (attrs.constGraph().numberOfNodes() > 0) {
            normalize_layout_translate(attrs);
            if (attrs.constGraph().numberOfEdges() > 0) {
                normalize_layout_scale_connected(attrs);
            } else if (attrs.constGraph().numberOfNodes() > 1) {
                normalize_layout_scale_disconnected(attrs);
            }
        }
        normalize_node_shapes(attrs);
    }

}  // namespace msc
