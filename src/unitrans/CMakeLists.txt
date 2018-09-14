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

add_executable(randiso randiso.cxx)
target_link_libraries(randiso PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-randiso-1st COMMAND ./randiso --help)
add_test(NAME clitest-randiso-2nd COMMAND ./randiso --version)
add_test(NAME clitest-randiso-3rd COMMAND ./randiso -o STDIO "${TEST_LAYOUT_FILE}")

add_executable(flip-nodes flip-nodes.cxx)
target_link_libraries(flip-nodes PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-flip-nodes-1st COMMAND ./flip-nodes --help)
add_test(NAME clitest-flip-nodes-2nd COMMAND ./flip-nodes --version)
add_test(NAME clitest-flip-nodes-3rd COMMAND ./flip-nodes -r 0.35 -o STDIO "${TEST_LAYOUT_FILE}")

add_executable(flip-edges flip-edges.cxx)
target_link_libraries(flip-edges PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-flip-edges-1st COMMAND ./flip-edges --help)
add_test(NAME clitest-flip-edges-2nd COMMAND ./flip-edges --version)
add_test(NAME clitest-flip-edges-3rd COMMAND ./flip-edges -r 0.35 -o STDIO "${TEST_LAYOUT_FILE}")

add_executable(movlsq movlsq.cxx)
target_link_libraries(movlsq PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-movlsq-1st COMMAND ./movlsq --help)
add_test(NAME clitest-movlsq-2nd COMMAND ./movlsq --version)
add_test(NAME clitest-movlsq-3rd COMMAND ./movlsq -r 0.35 -o STDIO "${TEST_LAYOUT_FILE}")

add_executable(perturb perturb.cxx)
target_link_libraries(perturb PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-perturb-1st COMMAND ./perturb --help)
add_test(NAME clitest-perturb-2nd COMMAND ./perturb --version)
add_test(NAME clitest-perturb-3rd COMMAND ./perturb -r 0.35 -o STDIO "${TEST_LAYOUT_FILE}")

add_executable(rotate rotate.cxx)
target_link_libraries(rotate PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-rotate-1st COMMAND ./rotate --help)
add_test(NAME clitest-rotate-2nd COMMAND ./rotate --version)
add_test(NAME clitest-rotate-3rd COMMAND ./rotate -r 0.35 -o STDIO "${TEST_LAYOUT_FILE}")
