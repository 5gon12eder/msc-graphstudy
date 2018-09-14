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

add_test(
    NAME doctest-driver
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${Python3_EXECUTABLE}" -m driver.doctests
)

add_test(
    NAME systest-driver-help
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${Python3_EXECUTABLE}" -m driver.deploy --help
)

add_test(
    NAME systest-driver-version
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${Python3_EXECUTABLE}" -m driver.deploy --version
)

add_test(
    NAME systest-driver-setup
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory _systest
)

add_test(
    NAME systest-driver-all
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
            "${Python3_EXECUTABLE}" -m driver.deploy
            "--configdir=${PROJECT_SOURCE_DIR}/test/config/"
            "--datadir=${PROJECT_BINARY_DIR}/_systest/output/"
            "--bindir=${PROJECT_BINARY_DIR}/"
            --log-level=INFO
            --all
)

add_test(
    NAME systest-driver-troll
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "_systest/output/graphs/carbon/"
)

add_test(
    NAME systest-driver-llort
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "_systest/output/graphs/carbon/"
)

add_test(
    NAME systest-driver-integrity-1st
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
            "${Python3_EXECUTABLE}" -m driver.integrity
            "--configdir=${PROJECT_SOURCE_DIR}/test/config/"
            "--datadir=${PROJECT_BINARY_DIR}/_systest/output/"
            "--bindir=${PROJECT_BINARY_DIR}/"
            --log-level=INFO
)

add_test(
    NAME systest-driver-integrity-2nd
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
            "${Python3_EXECUTABLE}" -m driver.integrity
            "--configdir=${PROJECT_SOURCE_DIR}/test/config/"
            "--datadir=${PROJECT_BINARY_DIR}/_systest/output/"
            "--bindir=${PROJECT_BINARY_DIR}/"
            --log-level=INFO
)
set_tests_properties(systest-driver-integrity-2nd PROPERTIES WILL_FAIL TRUE)

add_test(
    NAME systest-driver-check
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/_systest/output/"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/test/driver-check.py"
)

add_test(
    NAME systest-driver-cleanup
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
            "${Python3_EXECUTABLE}" -m driver.deploy
            "--configdir=${PROJECT_SOURCE_DIR}/test/data/config/"
            "--datadir=${PROJECT_BINARY_DIR}/_systest/output/"
            "--bindir=${PROJECT_BINARY_DIR}"
            --log-level=INFO
            -cc
)

set_tests_properties(systest-driver-all           PROPERTIES DEPENDS systest-driver-setup)
set_tests_properties(systest-driver-check         PROPERTIES DEPENDS systest-driver-all)
set_tests_properties(systest-driver-integrity-1st PROPERTIES DEPENDS systest-driver-all)
set_tests_properties(systest-driver-troll         PROPERTIES DEPENDS systest-driver-integrity-1st)
set_tests_properties(systest-driver-integrity-2nd PROPERTIES DEPENDS systest-driver-troll)
set_tests_properties(systest-driver-llort         PROPERTIES DEPENDS systest-driver-integrity-2nd)

set_tests_properties(systest-driver-setup   PROPERTIES FIXTURES_SETUP   fixture-data)
set_tests_properties(systest-driver-cleanup PROPERTIES FIXTURES_CLEANUP fixture-data)
set_tests_properties(systest-driver-troll   PROPERTIES FIXTURES_SETUP   fixture-troll)
set_tests_properties(systest-driver-llort   PROPERTIES FIXTURES_CLEANUP fixture-troll)

set_tests_properties(
    systest-driver-all
    systest-driver-check
    systest-driver-integrity-1st
    systest-driver-troll
    systest-driver-llort
    PROPERTIES FIXTURES_REQUIRED "fixture-data"
)

set_tests_properties(
    systest-driver-integrity-2nd
    PROPERTIES FIXTURES_REQUIRED "fixture-data;fixture-troll"
)

set_tests_properties(
    systest-driver-setup
    systest-driver-all
    systest-driver-check
    systest-driver-integrity-1st
    systest-driver-integrity-2nd
    systest-driver-troll
    systest-driver-llort
    systest-driver-cleanup
    PROPERTIES RESOURCE_LOCK "reslck-datadir"
)
