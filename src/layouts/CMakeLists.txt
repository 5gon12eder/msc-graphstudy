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

add_executable(force force.cxx)
target_link_libraries(force PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-force-1st COMMAND ./force --help)
add_test(NAME clitest-force-2nd COMMAND ./force --version)
add_test(NAME clitest-force-3rd COMMAND ./force -a FMMM "${TEST_GRAPH_FILE}")
add_test(NAME clitest-force-4th COMMAND ./force -a STRESS "${TEST_GRAPH_FILE}")
add_test(NAME clitest-force-5th COMMAND ./force -a DAVIDSON_HAREL "${TEST_GRAPH_FILE}")
add_test(NAME clitest-force-6th COMMAND ./force -a SPRING_EMBEDDER_KK "${TEST_GRAPH_FILE}")
add_test(NAME clitest-force-7th COMMAND ./force -a PIVOT_MDS "${TEST_GRAPH_FILE}")

add_executable(sugiyama sugiyama.cxx)
target_link_libraries(sugiyama PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME clitest-sugiyama-1st COMMAND ./sugiyama --help)
add_test(NAME clitest-sugiyama-2nd COMMAND ./sugiyama --version)
add_test(NAME clitest-sugiyama-3rd COMMAND ./sugiyama "${TEST_GRAPH_FILE}")

add_executable(random random.cxx)
target_link_libraries(random PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-random-1st COMMAND ./random --help)
add_test(NAME clitest-random-2nd COMMAND ./random --version)
add_test(NAME clitest-random-3rd COMMAND ./random "${TEST_GRAPH_FILE}")

add_executable(phantom phantom.cxx)
target_link_libraries(phantom PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-phantom-1st COMMAND ./phantom --help)
add_test(NAME clitest-phantom-2nd COMMAND ./phantom --version)
add_test(NAME clitest-phantom-3rd COMMAND ./phantom "${TEST_GRAPH_FILE}")
