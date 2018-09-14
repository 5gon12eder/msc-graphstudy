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

foreach(comp ${COMMON_COMPONENTS})
    # Replace any `/` with a `-` so CMake accepts the target name
    string(REPLACE "/" "-" compid ${comp})
    add_executable("test-${compid}" "${comp}.cxx")
    target_include_directories("test-${compid}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries("test-${compid}" PRIVATE common x-ta x-ut)
    add_test(NAME "unittest-${compid}" COMMAND "./test-${compid}")
endforeach(comp)
