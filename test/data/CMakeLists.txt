#! /usr/bin/cmake -P
#! -*- coding:utf-8; mode:cmake; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

# We cannot set the variables `TEST_GRAPH_FILE` and `TEST_LAYOUT_FILE` here because we already need them in the `src`
# subdirectory for running the CLI tests.  So we set them in the top-level `CMakeLists.txt` file instead.

set(graphml_files_to_validate
    test/data/adamantane-layout.xml
    test/data/cube-graph.xml
    test/data/cube-layout.xml
    test/data/julis-layout.xml
    test/data/kit-layout.xml
    test/data/mansion-layout.xml
    test/data/zigzag-layout.xml
)
