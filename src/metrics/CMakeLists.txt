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

add_executable(stress stress.cxx)
target_link_libraries(stress PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-stress-1st COMMAND ./stress --help)
add_test(NAME clitest-stress-2nd COMMAND ./stress --version)
add_test(NAME clitest-stress-3rd COMMAND ./stress "${TEST_LAYOUT_FILE}")
add_test(NAME clitest-stress-4th COMMAND ./stress --fit-nodesep "${TEST_LAYOUT_FILE}")
add_test(NAME clitest-stress-5th COMMAND ./stress --fit-scale "${TEST_LAYOUT_FILE}")

add_executable(huang huang.cxx)
target_link_libraries(huang PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-huang-1st COMMAND ./huang --help)
add_test(NAME clitest-huang-2nd COMMAND ./huang --version)
add_test(NAME clitest-huang-3rd COMMAND ./huang "${TEST_LAYOUT_FILE}")
