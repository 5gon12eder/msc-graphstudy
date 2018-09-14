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

add_executable(rawdata rawdata.cxx)
target_link_libraries(rawdata PRIVATE common ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-rawdata-1st COMMAND ./rawdata --help)
add_test(NAME clitest-rawdata-2nd COMMAND ./rawdata --version)
add_test(NAME clitest-rawdata-3rd COMMAND ./rawdata -k BOXED "${TEST_RAWDATA_FILE}")

add_executable(rdf-global rdf-global.cxx)
target_link_libraries(rdf-global PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-rdf-global-1st COMMAND ./rdf-global --help)
add_test(NAME clitest-rdf-global-2nd COMMAND ./rdf-global --version)
add_test(NAME clitest-rdf-global-3th COMMAND ./rdf-global -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")

add_executable(rdf-local rdf-local.cxx)
target_link_libraries(rdf-local PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-rdf-local-1st COMMAND ./rdf-local --help)
add_test(NAME clitest-rdf-local-2nd COMMAND ./rdf-local --version)
add_test(NAME clitest-rdf-local-3th COMMAND ./rdf-local -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")

add_executable(angular angular.cxx)
target_link_libraries(angular PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-angular-1st COMMAND ./angular --help)
add_test(NAME clitest-angular-2nd COMMAND ./angular --version)
add_test(NAME clitest-angular-3th COMMAND ./angular -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")

add_executable(edge-length edge-length.cxx)
target_link_libraries(edge-length PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-edge-length-1st COMMAND ./edge-length --help)
add_test(NAME clitest-edge-length-2nd COMMAND ./edge-length --version)
add_test(NAME clitest-edge-length-3th COMMAND ./edge-length -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")

add_executable(princomp princomp.cxx)
target_link_libraries(princomp PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-princomp-1st COMMAND ./princomp --help)
add_test(NAME clitest-princomp-2nd COMMAND ./princomp --version)
add_test(NAME clitest-princomp-3th COMMAND ./princomp -1 -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")
add_test(NAME clitest-princomp-4th COMMAND ./princomp -2 -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")

add_executable(tension tension.cxx)
target_link_libraries(tension PRIVATE common ogdf ${Boost_PROGRAM_OPTIONS_LIBRARIES})
add_test(NAME clitest-tension-1st COMMAND ./tension --help)
add_test(NAME clitest-tension-2nd COMMAND ./tension --version)
add_test(NAME clitest-tension-3th COMMAND ./tension -k BOXED -o STDIO -m STDIO "${TEST_LAYOUT_FILE}")
