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

add_executable(import import.cxx)
target_link_libraries(import PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-import-1st COMMAND ./import --help)
add_test(NAME clitest-import-2nd COMMAND ./import --version)
add_test(NAME clitest-import-3rd COMMAND ./import -o STDIO -f GraphML -m STDIO "${TEST_GRAPH_FILE}")
add_test(NAME clitest-import-4th COMMAND ./import -o STDIO -f GraphML -m STDIO "${TEST_LAYOUT_FILE}")
add_test(NAME clitest-import-5th COMMAND ./import -o STDIO -f GraphML -m STDIO --layout=0 "${TEST_LAYOUT_FILE}")
add_test(NAME clitest-import-6th COMMAND ./import -o STDIO -f GraphML -m STDIO --layout=1 "${TEST_LAYOUT_FILE}")

add_executable(lindenmayer lindenmayer.cxx)
target_link_libraries(lindenmayer PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-lindenmayer-1st COMMAND ./lindenmayer --help)
add_test(NAME clitest-lindenmayer-2nd COMMAND ./lindenmayer --version)
add_test(NAME clitest-lindenmayer-3rd COMMAND ./lindenmayer -n 100 -o STDIO)

add_executable(quasi quasi.cxx)
target_link_libraries(quasi PUBLIC common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-quasi-1st COMMAND ./quasi --help)
add_test(NAME clitest-quasi-2nd COMMAND ./quasi --version)
add_test(
    NAME clitest-quasi-3rd
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=6YXLeYsv9Nue" ./quasi -h 3 -n 100 -o STDIO
)
add_test(
    NAME clitest-quasi-4rd
    COMMAND "${CMAKE_COMMAND}" -E env "MSC_RANDOM_SEED=rJ4XHsBPzsnY" ./quasi -h 4 -n 100 -o STDIO
)

add_executable(grid grid.cxx)
target_link_libraries(grid PUBLIC common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-grid-1st COMMAND ./grid --help)
add_test(NAME clitest-grid-2nd COMMAND ./grid --version)
add_test(NAME clitest-grid-3rd COMMAND ./grid -n 100 -o STDIO)

add_executable(mosaic mosaic.cxx)
target_link_libraries(mosaic PUBLIC common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-mosaic-1st COMMAND ./mosaic --help)
add_test(NAME clitest-mosaic-2nd COMMAND ./mosaic --version)
add_test(NAME clitest-mosaic-3rd COMMAND ./mosaic -n 100 -o STDIO)

add_executable(bottle bottle.cxx)
target_link_libraries(bottle PUBLIC common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-bottle-1st COMMAND ./bottle --help)
add_test(NAME clitest-bottle-2nd COMMAND ./bottle --version)
add_test(NAME clitest-bottle-3rd COMMAND ./bottle -n 100 -o STDIO)

add_executable(tree tree.cxx)
target_link_libraries(tree PUBLIC common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-tree-1st COMMAND ./tree --help)
add_test(NAME clitest-tree-2nd COMMAND ./tree --version)
add_test(NAME clitest-tree-3rd COMMAND ./tree -n 100 -o STDIO)
